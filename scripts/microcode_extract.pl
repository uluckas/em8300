#!/usr/bin/perl

sub sanehead {
  my $blocktype = shift;
  my $offset = shift;
  my $dlen = shift;
  my $count = shift;

  if($blocktype > 0x300) { return 0 };
  if($offset > 0x10000) { return 0 };
  if($dlen > 20000) { return 0}
  return 1;
}

if($#ARGV < 0) {
  print "EM8300 microcode extractor\n";
  print "Written by Henrik Johansson 2000-04-10\n\n";
  print "Usage:\n\tmicrocode_extract.pl FILE [DESTDIR] [NAMEBASE]\n\n";
  print "FILE refers to the VXD file which comes with the Windows drivers
and is found in the Windows\\System folder. The file is usually called 
rmquasar.vxd (H+) or enc2dev.vxd (Creative Labs). \n\n";
  print "DESTDIR is the dir where the microcode should be stored.\n\n";
  print "NAMEBASE is the prefix of the saved microcode files.\n\n";
}

open(VXD,"<$ARGV[0]");
undef $/;
$vxd = <VXD>;
close(VXD);

$path=$ARGV[1];
$namebase=$ARGV[2];
if(length($namebase) == 0) { $namebase="microcode" };

$ucnt = 0;

for($i=0; $i < length($vxd); $i++) 
{
  if(unpack('v',substr($vxd,$i,2)) == 2) {
    $savei = $i;
    $ok = 1;
    $blocktype = -1;
    $count=0;
    while($blocktype != 0) {
      ($blocktype, $offset, $dlen) = unpack('vVV',substr($vxd,$i,11));
      
      if($blocktype == 0) { last; }
      
      if(($blocktype == 2) && (($lastblocktype&0xf00) == 0x200)) { $i--; last; }
      
      $dlen *= 2;
      if(!sanehead($blocktype,$offset,$dlen,$count)) {
	$ok = 0;
	last;
      }
      $i+=10+$dlen;
      $lastblocktype = $blocktype;
      $count++;
    }
    $lastblocktype = -1;      
    if($ok && $count > 20) {
      $ucodelen =  $i - $savei;
      $filename = "$path/$namebase$ucnt.bin";
      open UCODE,">$filename" or die("Can't open microcode file");
      print UCODE substr($vxd,$savei,$ucodelen);
      print pack('SII',(0,0,0));
      close UCODE;
      print "Found microcode block (length=$ucodelen), ";
      print "saving to $filename \n";
      $ucnt++;
    } else {
      $i = $savei;
    }
  }
}
