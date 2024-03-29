#!/usr/bin/perl

# msexpand written by Paul Laufer, 2001, to help him learn perl ;)
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Microsoft compress file structure:
# Uses a 12 bit (4k) sliding window Lempel Ziv variant.
#
# header: 13 bytes
# 	int MAGIC1 = 0x44445a53; // "SZDD"
# 	int MAGIC2 = 0x3327f088;
# 	char MAGIC3 = 0x41;
# 	char last_char_of_filename; // offset 0x09
# 	unsigned short int size_low;
# 	unsigned short int size_high;
#
# data portion:
# Flag byte followed by eight data elements, for each of the eight flag bits.
# If a flag bit is 1, the corresponding element is a data byte (just copied).
# If the flag bit is 0, the corresponding element is a code, comprised of two
# bytes.  The two bytes are divided into two parts. The upper 12 bits are the
# offset into the 4k window, and the lower 4 bits are the length of the string,
# minus 3. Thus the string length is between 3 and 18 bytes.

# Flag byte   Eight data elements
# 10010011
# |||||||+----byte
# ||||||+-----byte
# |||||+------code
# ||||+-------code
# |||+--------byte
# ||+---------code
# |+----------code
# +-----------byte
#
# The window is wrapped, ie: with an offset of 4092 and a length of 10, you
# will get the last 4 bytes then the first 6 bytes of the window in the output.
#
# Of course, the window must be updated with expanded strings and bytes as they
# are read. Oh, yeah, the window needs to be initialized with spaces, not
# zeros.

$MAGIC1 = 0x44445a53;
$MAGIC2 = 0x3327f088;
$MAGIC3 = 0x41;

$WINSIZE  = 4096;	# Window Size
$HEADSIZE = 14;		# Size of file header

sub LENGTH {
  my $x = shift;
  return ($x & 0x0F) + 3;
}
sub OFFSET {
  my ($x1, $x2) = @_;
  return (((($x2 & 0xF0) << 4) + $x1 + 0x0010) & 0x0FFF)
}
sub WRAPFIX {
  my $x = shift;
  return ($x & ($WINSIZE - 1));
}
sub BITSET {
  my ($byte, $bit) = @_;
  return (($byte & (1<<$bit)) > 0);
}

# This sub directly translated from my C source. Runs slow in perl...
sub LZ_expand {
  my ($input, $size_uncomp) = @_;
  local ($curr_pos, $location, $bit_map, $byte1, $byte2);
  local ($window[$WINSIZE], $length, $counter, $x, $max);

  # initialize window to all spaces (cleaner way?)
  for($x = 0; $x < $WINSIZE; $x += 1) {
    $window[$x] = ' ';
  }

  $curr_pos = 0;
  $index = $HEADSIZE;
  while ($curr_pos < $size_uncomp) {
    $bit_map = unpack('C', substr($input, $index, 1));
    $index++;
    if ($index >= $size_orig) {
      return;
    }

    for ($counter = 0; $counter < 8; $counter++) {
      if(!BITSET($bit_map, $counter)) {
	# Its a code, so process
	($byte1, $byte2) = unpack('CC', substr($input, $index, 2));
	$index += 2;
	if ($index >= $size_orig) {
	  return;
	}

	$length = LENGTH($byte2);
	$location = OFFSET($byte1, $byte2);

	while ($length > 0) {
	  $byte1 = $window[WRAPFIX($location)];
	  $window[WRAPFIX($curr_pos)] = $byte1;
	  printf(OUTFILE "%c", $byte1);
	  $curr_pos++;
	  $location++;
	  $length--;
	}
      }
      else {
	# Its just a data byte
	$byte1 = unpack('C', substr($input, $index, 1));
	$index++;
	$window[WRAPFIX($curr_pos)] = $byte1;
	printf(OUTFILE "%c", $byte1);
	$curr_pos++;
      }
      if ($index >= $size_orig) {
	return;
      }
    }
  }
  return;
}

# start here

if ($#ARGV < 0) {
  print "Microsoft Compressed File Expander\n";
  print "Written by Paul Laufer 2001-03-13\n\n";
  print "Usage:\n\tmsexpand.pl FILE\n\n";
  print "Where FILE is a valid Microsoft Compressed file. Files of this type\nusually have the last letter replaced with an underscore, ie rmquasar.vx_.\nThe expanded file will be the original filename with the last underscore\nreplaced with the original letter, ie rmquasar.vxd.\n\n";
  exit(1);
}

open(INFILE, "<$ARGV[0]") or die "Can't open file for input:";
undef $/;
$input = <INFILE>;
close(INFILE);

($magic1, $magic2, $magic3, $lastchar, $size_low, $size_high) =
  unpack 'VVCa1vv', substr($input, 0, $HEADSIZE);

if ($magic1 != $MAGIC1 || $magic2 != $MAGIC2 || $magic3 != $MAGIC3) {
  print "Error: Input file is not a Microsoft Compress format.\n";
  exit(1);
}
else {
  print "Input file appears to be Microsoft Compress format, proceeding\n";
}

$outfile = $ARGV[0];
substr($outfile, -1, 1) = $lastchar;

print "Output filename = ", $outfile, "\n";
$size_uncomp = ($size_high << 0x10) + $size_low;
$size_orig = -s $ARGV[0];
print "Original file size: ", $size_uncomp, " bytes\n";
printf "Compression ratio: %.1f%%\n", $size_orig * 100 / $size_uncomp;

open(OUTFILE, ">$outfile") or die "Can't open file for output:";

# Time to start expanding the file
LZ_expand($input, $size_uncomp);

close(OUTFILE);
