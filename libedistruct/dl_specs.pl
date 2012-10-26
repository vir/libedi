#!/usr/bin/perl
use strict;
use warnings;
use LWP::UserAgent;
use Archive::Extract;

my $uri = 'http://www.unece.org/trade/untdid/download/d99a.zip';
my $fn; ($fn = $uri) =~ s#.*/##;

unless(-f $fn) {
	my $ua = LWP::UserAgent->new;
	my $resp = $ua->get($uri, ':content_file' => $fn);
	unless($resp->is_success) {
		die $resp->status_line;
	}
}

my $mainzip = Archive::Extract->new(archive => $fn);
print "Extracting main archive $fn\n";
$mainzip->extract or die "Can't extract $fn: $!";

foreach my $file(@{ $mainzip->files }) {
	next unless $file =~ /^(.*)\.zip$/;
	my $dir = lc $1;
	print "Extracting archive $file into $dir\n";
	mkdir $dir;
	Archive::Extract->new(archive => $file)->extract(to => $dir) or die "Can't extract $file into $dir";
} continue {
	unlink $file;
}



