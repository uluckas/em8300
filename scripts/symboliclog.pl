#!/usr/bin/perl

open(PB, "microcode_info.pl <$ARGV[0] |");
while (<PB>) {
  if (/^Name:/) {
    chomp;
    s/Name: //;
    $symboltable{hex($b) + 0x1000} = $_;
  }
  s/Offset: 0x([0-9a-f]+)/$b = $1/e;
}
close PB;

while (<STDIN>) {
  s/register 0x([0-9a-f]+)/
    {
     if ($symboltable{hex($1)}) {
       "register " . $symboltable{hex($1)} . " (0x$1)";
     }
     else {
       "register 0x$1";
     }
    }
  /e;
  #if(!/register 0x1f4[de]/) { print; }
  print;
}
