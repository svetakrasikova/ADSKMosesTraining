#!/usr/bin/perl -w
#
# $Id: giza2bal.pl 1562 2008-02-19 20:48:14Z redpony $
#Converts direct and inverted alignments into a more compact 
#bi-alignment format. It optionally reads the counting file 
#produced by giza containing the frequency of each traning sentence.
#
# ©2004 Marcello Federico, last change in November 2004
# ©2011–2012 Autodesk Development Sàrl
# Last modified on 14 Feb 2012 by Ventsislav Zhechev
#
# ChangeLog
# v2.1
# Now we are reading only Bzip2 files, but doing it directly via a Perl module.
#
# v2.0.2
# Added progress indicator printout.
# Turned on autoflush to STDOUT and STDERR.
#
# v2.0.1
# Fixed a bad return value in case a sentence mismatch is found.
# Sentences are now split into tokens in the same way the rest of the training scripts do this—only on space and tab, rather than all white space.
#
# v2.0
# Now IO is strictly UTF-8.
# ‘use strict’ added.
# Converted to stricter and cleaner Perl code.
#
############################

use strict;
use IO::Uncompress::Bunzip2 qw/$Bunzip2Error/;
use Encode qw/encode decode/;

binmode STDERR, ":encoding(utf-8)";
select STDERR;
$| = 1;
binmode STDOUT, ":encoding(utf-8)";
select STDOUT;
$| = 1;

my ($cnt,$dir,$inv)=();

while (my $w=shift @ARGV){
  $dir=shift(@ARGV), next if $w eq "-d";
  $inv=shift(@ARGV), next if $w eq "-i";
  $cnt=shift(@ARGV), next if $w eq "-c";
} 

my $lc = 0;

if (!$dir || !$inv){
	print  "usage: giza2bal.pl [-c <count-file>] -d <dir-align-file> -i <inv-align-file>\n"; 
	print  "input files can be also commands, e.g. -d \"gunzip -c file.gz\"\n";
	exit(0);
}

my $DIR = new IO::Uncompress::Bunzip2 $dir
or die "Cannot open $dir ($Bunzip2Error)\n";
my $INV = new IO::Uncompress::Bunzip2 $inv
or die "Cannot open $inv ($Bunzip2Error)\n";

my $CNT;
if ($cnt){
	$CNT = new IO::Uncompress::Bunzip2 $cnt
	or die "Cannot open $cnt ($Bunzip2Error)\n";
}


sub ReadBiAlign{
	my ($fd0,$fd1,$fd2)=@_;
	my ($dummy,$n);
	
	my $c = 0;
	if ($fd0) {
		chomp($c=decode "utf-8", scalar <$fd0>); ## count
		$dummy=decode "utf-8", scalar <$fd0>; ## header
		$dummy=decode "utf-8", scalar <$fd0>; ## header
		$c=1 if !$c;
	}
	
	$dummy=decode "utf-8", scalar <$fd1>; ## header
	chomp(my $s1=decode "utf-8", scalar <$fd1>);
	chomp(my $t1=decode "utf-8", scalar <$fd1>);
	
	$dummy=decode "utf-8", scalar <$fd2>; ## header
	chomp(my $s2=decode "utf-8", scalar <$fd2>);
	chomp(my $t2=decode "utf-8", scalar <$fd2>);
	
	my $a = []; my $b = [];
	++$lc;
	
	#get target statistics
	$n=1;
	$t1=~s/NULL \(\{((\s+\d+)*)\s+\}\)//;
	while ($t1=~s/(\S+)\s+\(\{((\s+\d+)*)\s+\}\)//){
		grep($a->[$_]=$n,split(' ',$2));
		++$n;
	}
	
	my $m=1;
	$t2=~s/NULL \(\{((\s+\d+)*)\s+\}\)//;
	while ($t2=~s/(\S+)\s+\(\{((\s+\d+)*)\s+\}\)//){
		grep($b->[$_]=$m,split(' ',$2));
		++$m;
	}
	
	my $M=split(' ',$s1);
	my $N=split(' ',$s2);
	
	if ($m != ($M+1) || $n != ($N+1)) {
		print STDERR "Sentence mismatch error! Line #$lc\n";
		$s1 = "ALIGN_ERR";
		$s2 = "ALIGN_ERR";
		$a=[];$b=[];
		for (my $j=1;$j<2;$j++){ $a->[$j]=1; }
		for (my $i=1;$i<2;$i++){ $b->[$i]=1; }
		return ($s1, $s2, $a, $b, $c);
	}
	
	for (my $j=1;$j<$m;$j++){
		$a->[$j]=0 if !$a->[$j];
	}
	
	for (my $i=1;$i<$n;$i++){
		$b->[$i]=0 if !$b->[$i];
	}
	
	
	return ($s1, $s2, $a, $b, $c);
}

my $skip=0;
my $ccc=0;
while(!eof($DIR)){
	
	if (my ($src,$tgt,$a,$b,$c) = ReadBiAlign($CNT,$DIR,$INV))
	{
		print STDERR "." if !(++$ccc % 10000);
		print STDERR "[giza2bal: $ccc]" if !($ccc % 500000);
		print "$c\n";
		print $#{$a}," $src \# @{$a}[1..$#{$a}]\n";
		print $#{$b}," $tgt \# @{$b}[1..$#{$b}]\n";
	}
	else{
		print "\n\n\n";
		print STDERR "!" if !(++$skip % 10000);
	}
};
print STDERR "skip=<$skip> counts=<$ccc>\n";

close $CNT if $cnt;
close $DIR;
close $INV;


1;