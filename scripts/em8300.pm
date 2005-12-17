package em8300;

require Exporter;
@ISA    = qw(Exporter);
@EXPORT = qw(em8300_open em8300_close em8300_write em8300_read
             em8300_getregister em8300_getstatus em8300_setbcs
             em8300_setaspectratio);

$_IOC_NRBITS    =  8;
$_IOC_TYPEBITS  =  8;
$_IOC_SIZEBITS  = 14;
$_IOC_DIRBITS   =  2;

$_IOC_NRMASK    = ((1 << $_IOC_NRBITS)   - 1);
$_IOC_TYPEMASK  = ((1 << $_IOC_TYPEBITS) - 1);
$_IOC_SIZEMASK  = ((1 << $_IOC_SIZEBITS) - 1);
$_IOC_DIRMASK   = ((1 << $_IOC_DIRBITS)  - 1);

$_IOC_NRSHIFT    = 0;
$_IOC_TYPESHIFT  = ($_IOC_NRSHIFT   + $_IOC_NRBITS);
$_IOC_SIZESHIFT  = ($_IOC_TYPESHIFT + $_IOC_TYPEBITS);
$_IOC_DIRSHIFT   = ($_IOC_SIZESHIFT + $_IOC_SIZEBITS);

sub _IOC {
  local ($dir, $type, $nr, $size) = @_;

  ((($dir)       << $_IOC_DIRSHIFT)  |
   ((ord($type)) << $_IOC_TYPESHIFT) |
   (($nr)        << $_IOC_NRSHIFT)   |
   (($size)      << $_IOC_SIZESHIFT));
}

sub EMCTL_IOCTL_INIT           { _IOC(0, 'C', 0, 8);     }
sub EMCTL_IOCTL_READREG        { _IOC(3, 'C', 1, 12);    }
sub EMCTL_IOCTL_WRITEREG       { _IOC(1, 'C', 2, 12);    }
sub EMCTL_IOCTL_GETSTATUS      { _IOC(2, 'C', 3, shift); }
sub EMCTL_IOCTL_SETBCS         { _IOC(1, 'C', 4, 12);    }
sub EMCTL_IOCTL_SETASPECTRATIO { _IOC(1, 'C', 5, 4);     }

sub em8300_open {
  open(EMDEV,"</dev/em8300") or die("Can't open device /dev/em8300: $!");
}

sub em8300_close {
  close(EMDEV);
}

sub em8300_write {
  # Prepare ioctl
  $initparams = pack("III", @_);
  ioctl(EMDEV, &EMCTL_IOCTL_WRITEREG, $initparams)
    or die("Write register error");
}

sub em8300_read {
  my $reg = shift;
  my $ucode = shift;

  # Prepare ioctl
  $initparams = pack("III", $reg, 0, $ucode);
  ioctl(EMDEV, &EMCTL_IOCTL_READREG, $initparams)
    or die("Read register error");
  ($reg, $val) = unpack("II", $initparams);
  return $val;
}

sub em8300_getregister {
  my $reg = shift;
  my $ucode = shift;

  # Prepare ioctl
  $initparams = pack("III", $reg, 0, $ucode);
  ioctl(EMDEV, &EMCTL_IOCTL_READREG, $initparams)
    or die("Read register error");
  ($reg, $val) = unpack("II", $initparams);
  return $reg;
}

sub em8300_getstatus {
  my $reply;
  $reply = ' ' x 1024;
  ioctl(EMDEV, &EMCTL_IOCTL_GETSTATUS(1024), $reply)
    or die("Get status error");
  $len = index($reply, chr(0));
  return substr($reply, 0, $len);
}

sub em8300_setbcs {
  my $params = pack("III", @_);
  ioctl(EMDEV, &EMCTL_IOCTL_SETBCS, $params)
    or die("Set BCS error");
}

sub em8300_setaspectratio {
  my $params = pack("I", @_);
  ioctl(EMDEV, &EMCTL_IOCTL_SETASPECTRATIO, $params)
    or die("Set aspectratio error");
}
