#!/usr/bin/perl

use IO::Handle;
STDERR->autoflush(1);

sub print_result {
  $dstr = "";
  foreach my $d (@data) {
    $dstr .= sprintf("0x%x,", $d)
  }
  if ($bitn) {
    print "bits=$bitn r/w=$rw slaveaddress=",
      sprintf("0x%x", $slaveaddr),
	" subaddress=", sprintf("0x%x", $subaddr),
	  " data=", $dstr, " sr=", sprintf("%x", $sr >> 1), "\n";
  }
}
$oepin = 0x3c3c;
$pin   = 0x1c;
$DATA  = 0x8;
$CLK   = 0x10;
$CLK2  = 0x2;

while (<>) {
  if (/Write/ && /register 0x1f4[de]/) {
#   print;
    s/register (0x1f4[de])/$reg = hex($1);/e;
    s/Write 0x([0-9a-f]+)/$val = hex($1)/e;

    if ($reg == 0x1f4d) {
      $en = $val >> 8;
      $pin = ~$en & $pin | $en & $val & 0xff;
    }
    else {
      $en = $val >> 8;
      $oepin = ~$en & $oepin | $en & $val & 0xff;
    }

    $outpin = ~$oepin | $pin;

    $data = $outpin & $DATA ? 1 : 0;
    $data = $oepin  & $DATA ? $data : 'Z';

    $clk = $outpin & $CLK ? 1 : 0;

    if ($clk && (!$data && $lastdata)) {
      $outbit = 'S';
    }
    elsif ($clk && ($data && !$lastdata)) {
      $outbit = 'P';
    }
    elsif (!$clk && $lastclk && ($outbit ne 'S')) {
      $outbit = $data;
    }
    else {
      $outbit = '';
    }

    print $outbit;
    if ($outbit eq 'S') {
      undef @data;
      $bitn = 0;
    }
    elsif ($outbit eq 'P') {
      print "\n";
      &print_result;
    }
    elsif ($outbit ne '') {
      $sr <<= 1;
      $sr |= $outbit;

      if(($sr & 0xc060100) == 0xc060000) {

	printf " %02X %02X SR = 0x%08X\n", ($sr >> 9) & 0xff, ($sr & 0xff),$sr;
	$sr = 0;
      }
      {
	if ($bitn == 6) {
	  $slaveaddr = $sr & 0x7f;
	  last;
	}
	if ($bitn == 7) {
	  $rw = $lastbusdata;
	  last;
	}
	if ($bitn == 16) {
	  $subaddr = $sr & 0xff;
	  last;
	}
	if ($bitn >= 25 && (($bitn - 25) % 9) == 0) {
	  push(@data, $sr & 0xff);
	  last;
	}
      }

      $bitn++;
    }

    $lastclk  = $clk;
    $lastdata = $data;
  }
  else {
    if (!/0x1f4d/) {
      print;
    }
  }
}

&print_result;
