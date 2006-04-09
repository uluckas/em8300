#!/usr/bin/perl

$read_hreg_reg   = "EDX";
$read_value_reg  = "EAX";
$write_hreg_reg  = "EDX";
$write_value_reg = "EAX";

open(F, "<$ARGV[0]");
while (<F>) {
  s/EIP=[0-9A-F]+:([0-9A-F]+)/$sr{$1} = 1/e;
  @sra = keys(%sr);
  if ($#sra > 0) {
    break;
  }
}

@sra = sort @sra;

print $#sra, "\n";

$read  = $sra[0];
$write = $sra[1];

if ($#sra == 0) {
  $write = $sra[0];
  $read = 0;
}

close(F);

open(F, "<$ARGV[0]");
while (<F>) {

  if (/Break/) {
    $break =1;
    $op = "";
    next;
  }
  if (/GS/) {
    $break = 0;
    if ($ip eq $write) {
      print "Write 0x", sprintf("%x", $regs{$write_value_reg}),
        " to register 0x", sprintf("%x", $regs{$write_hreg_reg}), "\n";
    }
    if ($ip eq $read) {
      print "Read 0x", sprintf("%x", $regs{$read_value_reg}),
        " from register 0x", sprintf("%x", $regs{$read_hreg_reg}), "\n";
    }
    next;
  }

  if ($break) {
    s/EIP=[0-9A-F]+:([0-9A-F]+)/$ip = $1/e;
    s/(E[ABCD]X)=([0-9A-F]+)/$regs{$1} = hex($2)/eg;
    if (/^:/) {
      print;
    }
  }
}
