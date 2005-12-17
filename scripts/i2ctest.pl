#!/usr/bin/perl

use Socket;
use IO::Handle;
use em8300;

sub write_register {
  my $addr = shift;
  my $data = shift;

  if ($debugio) {
    printf "Write 0x%04x to register 0x%x ", $data, $addr;
  }

  em8300_write($addr, $data);

  if ($debugio) {
    $debugio = 0;
    printf ("I2C %x", (read_register(0x1f4d) & 0x1800) >> 8);
    print "\n";
    $debugio = 1;
  }
}

sub read_register {
  my $addr = shift;
  my $data;
  $data = em8300_read($addr);
  if ($debugio) {
    printf "Read 0x%04x from register 0x%x\n", $data, $addr;
  }
  return $data;
}

sub getline {
  my $line;
  while ($line !~ /\n/) {
    $line = <SOCK>;
  }
  return $line;
}

sub I2C_data {
  my $val = shift;
  write_register(0x1f4d, 0x800 | ($val ? 8 : 0));
}

sub I2C_read_data {
  my $val = (read_register(0x1f4d) & 0x800) ? 1 : 0;
  return $val;
}

sub I2C_drivedata {
  my $val = shift;
  write_register(0x1f4e, 0x800 | ($val ? 8 : 0));
}

sub I2C_driveclk {
  my $val = shift;
  write_register(0x1f4e, 0x1000 | ($val ? 0x10 : 0));
}

sub I2C_clk {
  my $val = shift;
  if ($useclk == 2) {
    write_register(0x1f4d, 0x2000 | ($val ? 0x20 : 0));
  }
  else {
    write_register(0x1f4d, 0x1000 | ($val ? 0x10 : 0));
  }
}

sub I2C_out {
  my $data = shift;
  my $bits = shift;

  for (my $i = $bits - 1; $i >= 0; $i--) {
    I2C_data($data & (1 << $i));
    I2C_clk(1);
    I2C_clk(0);
  }
}

sub I2C_in {
  my $bits = shift;
  my $data;

  for (my $i = $bits - 1; $i >= 0; $i--) {
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

  I2C_out($slaveaddr, 7);	# slave address
  I2C_out(0, 1);		# R/W
  &I2C_ack or return -1;	# Ack

  I2C_out($subaddr, 8);		# slave address
  &I2C_ack or return -1;	# Ack

  while ($#_ >= 0) {
    I2C_out(shift, 8);		# data
    &I2C_ack or return -1;	# Ack
  }

  # Stopbit
  &I2C_stop;

  return 0;
}

sub I2C_ack {
  I2C_drivedata(0);
  for (my $count = 0; $count < 256; $count++) {
    if (!I2C_read_data()) {
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

  I2C_out($slaveaddr, 7);	# slave address
  I2C_out(0, 1);		# R/W
  &I2C_ack or return 0;	        # Ack


  I2C_out($subaddr, 8);		# sub address
  &I2C_ack or return 0;	        # Ack

  # Startbit
  I2C_start;

  I2C_out($slaveaddr, 7);	# slave address
  I2C_out(1, 1);		# R/W
  &I2C_ack or return 0;	        # Ack

  for (my $i = 0; $i < $n; $i++) {
    push(@data, I2C_in(8));	# data
    I2C_out(1, 1);
  }

  &I2C_stop;

  return @data;
}

sub write_myst {
  my $data = shift;

  I2C_data(1);
  for(my $i = 0; $i < 8; $i++) {
    write_register(0x1f4d, 0x2000);
    I2C_data($data & 1);
    write_register(0x1f4d, 0x2020);
    $data >>= 1;
  }
  write_register(0x1f4d, 0x0200);
  write_register(0x1f4d, 0x0202);
  I2C_data(1);
}


sub sub_23660 {
  local ($arg1, $arg2) = @_;
  I2C_clk(0);
  I2C_out($arg1, 8);
  I2C_data($arg2);
  I2C_clk(1);
}

sub sub_236f0 {
  local ($arg1, $arg2, $arg3)=@_;
  I2C_clk(1);
  I2C_data(1);
  I2C_clk(0);
  I2C_data(1);
  I2C_clk(1);
  I2C_clk(0);

  sub_23660(1, $arg2);

  sub_23660($arg1, $arg3);
}

sub sub_237e0 {
  local ($arg1, $arg2, $arg3) = @_;
  sub_236f0($arg1, 1, 0);
  sub_23660($arg2, 1);
}

sub myst_in {
  my $bits = shift;
  my $data;

  for (my $i = $bits - 1; $i >= 0; $i--) {
    $data |= I2C_read_data() << $i;
    I2C_clk(0);
    I2C_clk(1);
  }
  return $data;
}

sub sub_23780 {
  local ($arg1) = @_;

  sub_236f0($arg1, 0, 0);
  I2C_drivedata(0);
  my $val = myst_in(8);
  I2C_drivedata(1);
  I2C_clk(0);
  I2C_data(1);
  I2C_clk(1);
  return $val;

}

sub reset {
  write_register(0x1f50, 0x123);
  read_register(0x1c08);
  write_register(0x1f4d, 0x3f3f);
  write_register(0x1f4e, 0x3b3b);
  write_register(0x1f4d, 0x0100);
  write_register(0x1f4d, 0x0101);
  write_register(0x1f4d, 0x0808);
}

sub connect_server {
  em8300_open();

  &reset;
  write_register(0x1f4d, 0x3c3c);
  write_register(0x1f4e, 0x3c00);
  write_register(0x1f4e, 0x3c3c);

  $bars = 0;
  $debugio = 0;

  $useclk = 1;

  I2C_data(1);
  I2C_clk(1);

  write_register(0x1274, 0x8000);

  I2C_write(0x6a, 0,
	    0x11,	# Mode Register 0
	    0x0 | ($bars ? 0x80 : 0) , # Mode Register 1
	    0xcb,	# Subcarrier Frequency Register 0
	    0x8a,	# Subcarrier Frequency Register 1
	    0x09,	# Subcarrier Frequency Register 2
	    0x2a,	# Subcarrier Frequency Register 3
	    0x00,	# Subcarrier Phase Register
	    0x45,	# Timing Register 0
	    0x00,	# Closed Captioning Ext Register 0
	    0x00,	# Closed Captioning Ext Register 1
	    0x00,	# Closed Captioning Register 0
	    0x00,	# Closed Captioning Register 1
	    0x73,	# Timing Register 1
	    0x08,	# Mode Register 2
	    0x00,	# Pedestal Control Register 0
	    0x00,	# Pedestal Control Register 1
	   ) != -1 or die("I2C protocol error");


# NTSC
# print I2C_write(0x6a, 0x0,  0x20, 0x7f, 0x16, 0x7c, 0xf0, 0x21,
#		  0x0,  0x45, 0x0,  0x0,  0x0,  0x0,  0x73, 0x8);

# I2C_write(0x6a, 0x12, 0x40, 0x80, 0x16, 0xaa, 0x61, 0x5, 0xd7, 0x53,
#	    0xfe, 0x3,  0xaa, 0x80, 0xbf, 0x1f, 0x18, 0x4, 0x7a, 0x55), "\n";

#  write_myst(0xb3);
#  write_myst(0xbb);

  I2C_write(0x6a, 0x7, 0x4d);
  I2C_write(0x6a, 0x7, 0xcd);
  I2C_write(0x6a, 0x7, 0x4d);

  write_register(0x1274, 0x4001);

# I2C_write(0x6a, 0x7, 0x4d);

  print "d:", read_register(0x1f4b), "\n";

# sub_237e0(7, 0);

  em8300_close();
}

&connect_server;
