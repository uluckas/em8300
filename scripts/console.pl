#!/usr/bin/perl
use IO::Handle;
use em8300;

%microcode_registers = (
			mv_command,0,
			mv_status,1,
			mv_buffstart_lo,2,
			mv_buffstart_hi,3,
			mv_buffsize_lo,4,
			mv_buffsize_hi,5,
			mv_rdptr_lo,6,
			mv_rdptr_hi,7,
			mv_threshold,8,
			mv_wrptr_lo,9,
			mv_wrptr_hi,10,
			mv_pcirdptr,11,
			mv_pciwrptr,12,
			mv_pcisize,13,
			mv_pcistart,14,
			mv_ptsrdptr,15,
			mv_ptssize,16,
			mv_ptsfifo,17,
			mv_scrspeed,18,
			mv_scrlo,19,
			mv_scrhi,20,
			mv_framecntlo,21,
			mv_framecnthi,22,
			mv_frameeventlo,23,
			mv_frameeventhi,24,
			mv_accspeed,25,
			width_buf3,26,
			ma_command,27,
			ma_status,28,
			ma_buffstart_lo,29,
			ma_buffstart_hi,30,
			ma_buffsize,31,
			ma_buffsize_hi,32,
			ma_rdptr,33,
			ma_rdptr_hi,34,
			ma_threshold,35,
			ma_wrptr,36,
			ma_wrptr_hi,37,
			q_irqmask,38,
			q_irqstatus,39,
			q_intcnt,40,
			ma_pcirdptr,41,
			ma_pciwrptr,42,
			ma_pcisize,43,
			ma_pcistart,44,
			sp_command,45,
			sp_status,46,
			sp_buffstart_lo,47,
			sp_buffstart_hi,48,
			sp_buffsize_lo,49,
			sp_buffsize_hi,50,
			sp_rdptr_lo,51,
			sp_rdptr_hi,52,
			sp_wrptr_lo,53,
			sp_wrptr_hi,54,
			sp_pcirdptr,55,
			sp_pciwrptr,56,
			sp_pcisize,57,
			sp_pcistart,58,
			sp_ptsrdptr,59,
			sp_ptssize,60,
			sp_ptsfifo,61,
			dicom_displaybuffer,62,
			vsync_dbuf,63,
			dicom_tvout,64,
			dicom_updateflag,65,
			dicom_vsynclo1,66,
			dicom_vsynclo2,67,
			dicom_vsyncdelay1,68,
			dicom_vsyncdelay2,69,
			dicom_display_data,70,
			picptslo,71,
			picptshi,72,
			error_code,73,
			displayhorsize,74,
			line21buf1_cnt,75,
			line21buf2_cnt,76,
			timecodehi,77,
			timecodelo,78,
			auth_challenge,79,
			auth_response,80,
			auth_command,81,
			timer_cnt,82,
			ovl_addr,83,
			button_color,84,
			button_contrast,85,
			button_top,86,
			button_bottom,87,
			button_left,88,
			button_right,89,
			sp_palette,90,
			dicom_frametop,91,
			dicom_framebottom,92,
			dicom_frameleft,93,
			dicom_frameright,94,
			dicom_visibletop,95,
			dicom_visiblebottom,96,
			dicom_visibleleft,97,
			dicom_visibleright,98,
			dicom_bcsluma,99,
			dicom_bcschroma,100,
			dicom_control,101,
			dicom_controlx,102,
			mv_cryptkey,103,
			dicom_kmin,104,
			microcodeversion,105,
			forcedleftparity,106,
			l21_buf1,107,
			l21_buf2,108,
			mute_pattern,109,
			mute_patternrityhtm,110
);		       

sub write_register {
  my $addr = shift;
  my $data = shift;
  
  if($debugio) {
    printf "Write 0x%04x to register 0x%x ",$data,$addr;
  }
  
  em8300_write($addr,$data);
  
  if($debugio) {
    printf ("I2C %x", (read_register(0x1f4d) & 0x1800) >> 8);
    print "\n";
  }
}

sub read_register {
  my $addr = shift;
  my $data;
  return em8300_read($addr,$data);
}


sub I2C_data {
  my $val = shift;
  write_register(0x1f4d,0x800 | ($val ? 8 : 0));
}

sub I2C_read_data {
  I2C_drivedata(0);
  my $val = (read_register(0x1f4d) & 0x800) ? 1 : 0;
  I2C_drivedata(1);
  return $val;
}

sub I2C_drivedata {
  my $val = shift;
  write_register(0x1f4e,0x800 | ($val ? 8 : 0));
}

sub I2C_clk {
  my $val = shift;
  if($useclk==2) {
    write_register(0x1f4d,0x2000 | ($val ? 0x20 : 0));
  } else {
    write_register(0x1f4d,0x1000 | ($val ? 0x10 : 0));
  }
}

sub I2C_out {
  my $data = shift;
  my $bits = shift;
  
  for(my $i=$bits-1; $i >= 0; $i--) {
    I2C_data($data & (1 << $i));
    I2C_clk(1);
    I2C_clk(0);

  }
}

sub I2C_in {
  my $bits = shift;
  my $data;
  
  for(my $i=$bits-1; $i >= 0; $i--) {
    $data |= I2C_read_data() << $i;
    I2C_clk(1);
    I2C_clk(0);
  }
  return $data;
}

sub I2C_start {
  I2C_data(1);
  I2C_clk(1);
  I2C_data(0);
  I2C_clk(0);
}

sub I2C_stop {
  I2C_clk(0);
  I2C_data(0);
  I2C_clk(1);
  I2C_data(1);
}


#
# I2c_write(slave_address, subaddress, data1, data2 ...)
#
sub I2C_write {
  $slaveaddr = shift;
  $subaddr = shift;
  
  &I2C_start;
  
  I2C_out($slaveaddr,7);	# slave address
  I2C_out(0,1);			# R/W
  &I2C_ack or return -1;	# Ack
  
  I2C_out($subaddr,8);		# slave address
  &I2C_ack or return -1;	# Ack
  
  while($#_ >= 0) {
    I2C_out(shift,8);		# data
    &I2C_ack or return -1;	# Ack
  }
  
  # Stopbit
  &I2C_stop;
  
  return 0;
}

sub I2C_ack {
    I2C_drivedata(0);
  for(my $count=0; $count < 256; $count++) {
    if(!I2C_read_data()) {
      I2C_drivedata(1);
      I2C_clk(1);
      I2C_clk(0);
      return 1;
    }
  }
  I2C_drivedata(1);
  return 0;
}

#
# I2c_read(slave_address, subaddress, # of bytes to read)
#
sub I2C_read {
  my $slaveaddr = shift;
  my $subaddr = shift;
  my $n = shift;
  my @data;
  
  &I2C_start;
  
  I2C_out($slaveaddr,7);	# slave address
  I2C_out(0,1);			# R/W
  &I2C_ack or return 0;	        # Ack

  
  I2C_out($subaddr,8);		# sub address
  &I2C_ack or return 0;	        # Ack
  
  # Startbit
  I2C_start;
  
  I2C_out($slaveaddr,7);	# slave address
  I2C_out(1,1);			# R/W
  &I2C_ack or return 0;	        # Ack
  
  for(my $i=0; $i < $n; $i++) {
    push(@data,I2C_in(8));	# data
    I2C_out(1,1);
  }
  
  &I2C_stop;
  
  return @data;
}

sub write_myst {
  my $data = shift;
  
  I2C_data(1);
  for(my $i=0; $i <8 ; $i++) {
    write_register(0x1f4d, 0x2000);
    I2C_data($data & 1);
    write_register(0x1f4d, 0x2020);
    $data >>= 1;
  }
  write_register(0x1f4d, 0x0200);
  write_register(0x1f4d, 0x0202);
}

sub reset {
  write_register(0x1f50,0x123);
  read_register(0x1c08);
  write_register(0x1f4d,0x3f3f);
  write_register(0x1f4e,0x3b3b);
  write_register(0x1f4d,0x0100);
  write_register(0x1f4d,0x0101);
  write_register(0x1f4d,0x0808);
}  


sub I2C_em9010_in {
  my $bits = shift;
  my $data;
  
  for(my $i=$bits-1; $i >= 0; $i--) {
    $data |= I2C_read_data() << $i;
    I2C_clk(1);
    I2C_clk(0);
  }
  return $data;
}

sub sub_23660 {
  local($arg1,$arg2)=@_;
  I2C_clk(0);
  I2C_out($arg1,8);
  I2C_data($arg2);  
  I2C_clk(1);
}

sub sub_236f0 {
  local($arg1,$arg2,$arg3)=@_;
  I2C_clk(1);
  I2C_data(1);
  I2C_clk(0);
  I2C_data(1);
  I2C_clk(1);
  I2C_clk(0);

  sub_23660(1,$arg2);

  sub_23660($arg1,$arg3);
}

# sub_237e0
sub em9010_write {
  local($arg1,$arg2,$arg3)=@_;
  sub_236f0($arg1,1,0);
  sub_23660($arg2,1);
}

# sub_23700
sub em9010_read {
  local($arg1)=@_;

  sub_236f0($arg1,0,0);
  I2C_drivedata(0);
  my $val = I2C_em9010_in(8);
  I2C_drivedata(1);
  I2C_clk(0);
  I2C_data(1);
  I2C_clk(1);
  return $val;
  
}

sub em9010_read16 {
  local($reg)=@_;
  
  if($reg > 128) {
    em9010_write(3,0);
    em9010_write(4,$reg);
  } else {
    em9010_write(4,0);
    em9010_write(3,$reg);
  }
  return em9010_read(2) | (em9010_read(1) << 8);
}

sub em9010_write16 {
  local($reg,$value)=@_;
  
  if($reg > 128) {
    em9010_write(3,0);
    em9010_write(4,$reg);
  } else {
    em9010_write(4,0);
    em9010_write(3,$reg);
  }

  em9010_write(2,$value & 0xff);
  em9010_write(1,$value >> 8);
}

sub resolve_register
{
  local $reg = shift;
  local $uc = shift;
  
  if($reg =~ /^[0-9a-fx]+$/) {
    $$uc = 0;
    return hex($reg);
  } elsif($microcode_registers{$reg} ne '') {
    $$uc = 1;
    return $microcode_registers{$reg};
  } else {
    return "error";
  }
}

sub usage {
  print "Valid Commands:\n\n";
  print "w <REGISTER> <VALUE>\tWrite to register. REGISTER\n\t\t\tis either a hexadecimal number or\n\t\t\ta symbolic register name. \n\t\t\tExample: w mv_command 3\n"; 
  print "r <REGISTER>\t\tRead from register.\n"; 
  print "ra\t\t\tRead out all registers.\n"; 
  print "l\t\t\tList available em8300 registers.\n"; 
  print "sw <REGISTER> <VALUE>\tWrite to mysterious serial device\n"; 
  print "bcs <BRIGHTNESS> <CONTRAST> <SATURATION>\n"; 
  print "ow <REGISTER> <DATA>\tWrite to overlay processor\n"; 
  print "ow16 <REGISTER> <DATA>\tWrite to overlay processor 16-bit register\n"; 
#  print "or <REGISTER>\t\tRead from overlay processor\n"; 
#  print "or16 <REGISTER>\t\tRead from overlay processor 16-bit\n"; 
  print "win <WIDTH> <HEIGHT> <XPOS> <YPOS>\tSet overlay window size/position\n"; 

  print "status\t\t\tGet status from device driver\n"; 
  print "q\t\t\tQuit\n"; 
  print "h\t\t\tPrint this message\n"; 
print "\n";
}

em8300_open;

#&reset;
write_register(0x1f4d,0x3c3c);
write_register(0x1f4e,0x3c00);
write_register(0x1f4e,0x3c3c);

$useclk=1;
I2C_data(1);
I2C_clk(1);

STDOUT->autoflush(1);

print "EM8300 Console\n\n";

&usage;
print "EM8300>";
while(<>) {
  chomp;
  $_ = lc();
  if(/^e/) { last; }
  elsif(/^sw /) { 
    s/sw ([0-9a-f]+)/ write_myst(hex($1));/e;
  } elsif(/^w (\w+) [0-9a-f]+/) {
    s/w (\w+) ([0-9a-f]+)/
      $reg = resolve_register($1,\$uc);
      if($reg ne 'error') {
	em8300_write($reg,hex($2),$uc);
      } else {
	print "Unknown register $1\n";
      }
    /e;
  } elsif(/^r \w+/) {
    s/r (\w+)/
      $reg = resolve_register($1,\$uc);
      if($reg ne 'error') {
	printf("0x%x\n", em8300_read($reg,$uc));
      } else {
	print "Unknown register $1.\n";
      }
    /e;
  } elsif(/^win [0-9]+ [0-9]+ [0-9]+ [0-9]+/) {
    s/^win ([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)/
      $width=$1;$height=$2;
      $xpos=$3;$ypos=$4;
    /e;    
    em8300_write($microcode_registers{dicom_frametop}, $ypos,1);
    em8300_write($microcode_registers{dicom_visibletop}, $ypos,1);
    em8300_write($microcode_registers{dicom_frameleft}, $xpos,1);
    em8300_write($microcode_registers{dicom_visibleleft}, $xpos,1);
    em8300_write($microcode_registers{dicom_frameright}, $xpos+$width,1);
    em8300_write($microcode_registers{dicom_visibleright}, $xpos+$width,1); 
    em8300_write($microcode_registers{dicom_framebottom}, $ypos+$height,1);
    em8300_write($microcode_registers{dicom_visiblebottom}, $ypos+$height,1); 
    em8300_write($microcode_registers{dicom_updateflag}, 1,1); 
    s/r ([0-9a-f]+)/ printf ("0x%x\n",em8300_read(hex($1),0));/e;
  } elsif(/^status/) {
    $status = &em8300_getstatus;
    $tmp = $status;
    $tmp =~ s/Time elapsed: ([0-9]+) us/$time=$1/e;
    $tmp =~ s/Frames: ([0-9]+)/$frames=$1/e;
    $tmp =~ s/SCR diff: ([0-9]+)/$scrdiff=$1/e;
    $tmp =~ s/SCR: ([0-9]+)/$scr=$1/e;
    $tmp =~ s/Picture PTS: ([0-9]+)/$picpts=$1/e;
    $lag = $scr-$picpts;
    print $status;
    if($time) {
      print "Picture-Clock Reference lag: ",$lag,"(",1/45000*$lag,")", "\n";
      print "FPS: ", $frames / ($time * 1e-6), "\n";
      print "SCRPS: ", $scrdiff / ($time * 1e-6), "\n";
    }
    print "\n";
  }
  elsif(/^ma/) { 
    printf "MA_Status  : %04x\n", em8300_read(0x111f);
    printf "MA_RdPtr   : %04x\n", em8300_read(0x1124) | (em8300_read(0x1125) << 16);
    printf "MA_WrPtr   : %04x\n", em8300_read(0x1127);
    printf "MA_PCIRdPtr: %04x\n", em8300_read(0x1130);
    printf "MA_PCIWrPtr: %04x\n", em8300_read(0x1131);
  }
  elsif(/^l/) {
    @regs = keys(%microcode_registers);
    @regs = sort(@regs);
    print join("\n",@regs);
  } 
  elsif(/^ra/) {
    @regs = keys(%microcode_registers);
    @regs = sort(@regs);
    foreach my $r (@regs) {
      $val = em8300_read($microcode_registers{$r},1);
      $reg = em8300_getregister($microcode_registers{$r},1);
      print sprintf("%-20s (0x%04x): 0x%04x",$r,$reg,$val), "\n";
    }
  } elsif(/^ow [0-9a-f]+ [0-9a-f]+/) {
    s/^ow ([0-9a-f]+) ([0-9a-f]+)/em9010_write(hex($1),hex($2))/e;
  } elsif(/^ow16 [0-9a-f]+ [0-9a-f]+/) {
    s/^ow16 ([0-9a-f]+) ([0-9a-f]+)/em9010_write16(hex($1),hex($2))/e;
  } elsif(/^or [0-9a-f]+/) {
    s/^or ([0-9a-f]+)/printf "%02x\n",em9010_read(hex($1))/e;
  } elsif(/^or16 [0-9a-f]+/) {
    s/^or16 ([0-9a-f]+)/printf "%02x\n",em9010_read16(hex($1))/e;
  } elsif(/^x/) {
    
    $displaybuffer=em8300_read($microcode_registers{dicom_displaybuffer},1) + 0x1000;
    $r1 = em8300_read($displaybuffer);
    $r2 = em8300_read($displaybuffer+1);
    $r3 = em8300_read($displaybuffer+2) & 0x1fff;

    $r4 = em8300_read($displaybuffer+3) |
      (em8300_read($displaybuffer+4) << 16);
    $r4 <<= 4;

    $r5 = em8300_read($displaybuffer+5) |
      (em8300_read($displaybuffer+6) << 16);
    $r5 <<= 4;
    
    printf("%d\n",$r1);
    printf("%d\n",$r2);
    printf("%d\n",$r3);
    printf("0x%x\n",$r4);
    printf("0x%x\n",$r5);

    
  } elsif(/^bcs/) {
    s/bcs ([0-9]+) ([0-9]+) ([0-9]+)/em8300_setbcs($1,$2,$3)/e;
  } elsif(/^ar [01]/) {
    s/ar ([01])/em8300_setaspectratio($1)/e;
  }elsif(/^q/) {
    exit(1);
  }
  elsif(/^h/) {
    &usage;
  } else {
    print "Unknown command\n";
  }

  print "EM8300>"
}

em8300_close
