#!/usr/bin/perl

use em8300;
use IO::Handle;

sub write_register {
  my $addr = shift;
  my $data = shift;

  if ($debugio) {
    printf "Write 0x%04x to register 0x%x ", $data, $addr;
  }

  em8300_write($addr, $data);

  if ($debugio) {
    printf ("I2C %x", (read_register(0x1f4d) & 0x1800) >> 8);
    print "\n";
  }
}

sub read_register {
  my $addr = shift;
  my $data;
  return em8300_read($addr, $data);
}

em8300_open;

while (<>) {
  if (/Write/ && /register/) {
    s/register (0x[0-9a-f]+)/$reg = hex($1);/e;
    s/Write 0x([0-9a-f]+)/$val = hex($1)/e;
    write_register($reg, $val);
  }
}

em8300_close;
