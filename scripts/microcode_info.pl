#!/usr/bin/perl

sub decrypt {
  my $data=shift;
  my $len=shift;
  my $outdata;
  my $chr;

  for(my $i=0; $i < $len; $i++) {
    $chr = ord(substr($data, $i, 1));
    if($chr) {
      $outdata .= chr($chr ^ 0xff);
    }
  }
  return $outdata;
}

sub print_binstr {
  my $chr;
  my $data=shift;
  my $len=shift;

  for(my $i=0; $i < $len; $i++) {
    printf("0x%02x,",ord(substr($data, $i, 1)));
    if( $i && !(($i+1) % 8) ) {
      print "\n";
    }
  }  
}

undef $/;
$ucode = <>;

while(1) {
  ($flags,$offset,$len) = unpack('SII',$ucode);
  $len*=2;

  printf("Flags:  0x%x\n",$flags);
  printf("Offset: 0x%x\n",$offset);
  printf("Len:    0x%x\n",$len);

  $data = substr($ucode,10,$len);
  
  if(!$flags) { last; } 
  
  if($flags & 0x200) {
    $data = decrypt($data,$len);
    print "Name: $data\n";
  }
  
  if(($flags & 0xf00) == 0) {
    print_binstr($data,$len);
  }
  
  print "\n";

  $ucode = substr($ucode,10+$len);
}
