$_IOC_NRBITS    =  8;
$_IOC_TYPEBITS  =  8;
$_IOC_SIZEBITS  = 14;
$_IOC_DIRBITS   =  2;

$_IOC_NRMASK    = ((1 << $_IOC_NRBITS)-1);
$_IOC_TYPEMASK  = ((1 << $_IOC_TYPEBITS)-1);
$_IOC_SIZEMASK  = ((1 << $_IOC_SIZEBITS)-1);
$_IOC_DIRMASK   = ((1 << $_IOC_DIRBITS)-1);

$_IOC_NRSHIFT    = 0;
$_IOC_TYPESHIFT  = ($_IOC_NRSHIFT+$_IOC_NRBITS);
$_IOC_SIZESHIFT  = ($_IOC_TYPESHIFT+$_IOC_TYPEBITS);
$_IOC_DIRSHIFT   = ($_IOC_SIZESHIFT+$_IOC_SIZEBITS);

sub _IOC {
  local ($dir,$type,$nr,$size) = @_;

  ((($dir)  << $_IOC_DIRSHIFT) | 
   ((ord($type)) << $_IOC_TYPESHIFT) | 
   (($nr)   << $_IOC_NRSHIFT) | 
   (($size) << $_IOC_SIZESHIFT));
}

sub EMCTL_IOCTL_INIT { _IOC(1,'C',0,8 );}
sub EMCTL_IOCTL_READREG { _IOC(3,'C',2,8);}
sub EMCTL_IOCTL_WRITEREG { _IOC(1,'C',1,8); }
sub EMCTL_IOCTL_GETSTATUS { _IOC(2,'C',3,shift)}

sub em8300_open {
  open(EMDEV,"</dev/em8300") or die("Can't open device");
}

sub em8300_close {
  close(EMDEV);
}

sub em8300_write {
  my $reg = shift;
  my $val = shift;
  my $ucode = shift;

  # Prepare ioctl
  $initparams = pack("III", $reg, $val,$ucode);
  ioctl(EMDEV, &EMCTL_IOCTL_WRITEREG, $initparams) or die("Write register error");
}

sub em8300_read {
  my $reg = shift;
  my $ucode = shift;

  # Prepare ioctl
  $initparams = pack("III", $reg, 0, $ucode);
  ioctl(EMDEV, &EMCTL_IOCTL_READREG, $initparams) or die("Read register error");
  ($reg, $val) = unpack("II",$initparams);
  return $val;
}

sub em8300_getstatus {
  my $reply;
  
  $reply = ' ' x 1024;

  ioctl(EMDEV, &EMCTL_IOCTL_GETSTATUS(1024), $reply) or die("Get status error");
  $len = index($reply,chr(0));
  return substr($reply,0,$len);
}
