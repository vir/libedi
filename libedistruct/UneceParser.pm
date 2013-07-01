package UneceParser;
use re 'eval';

sub load_segments_file
{
	my($fn, $r) = @_;
	$r ||= {};
	my $segment;
	my $lasttextref;
	my $in_composite = 0;
	my $change_sign = qr/[\+\*\#\|X\-]/;
	my $unused = '';
	open F, '<:bytes', $fn or die;
	while(<F>) {
		if(/^\s*$/) { # skip empty lines
			undef $lasttextref;
			next;
		}
		if(/^\xC4+/) { # horizontal ruler in win encoding
			$r->{delete $segment->{name}} = $segment if $segment && keys %$segment;
			$segment = {};
		} elsif(/^\s+(?:$change_sign )?([A-Z]{3}) {3}(\w.*?)\s*$/) { # "big" heading
			$segment->{name} = $1;
			$segment->{title} = $2;
			$lasttextref = \$segment->{title};
		} elsif(/^\s+Function:\s*(.*?)\s*$/) {
			$segment->{function} = $1;
			$lasttextref = \$segment->{function};
		} elsif(/^(\d+)\s+(?:$change_sign\s+)?(\d{4})\s+(\w.*?)\s+([MC])\s+(a?n?(?:\.\.)?\d*)\s*$/) { # element
#			push @{$segment->{elements}}, [$1, $2, $3, $4];
			push @{$segment->{elements}}, "$4$2";
			$lasttextref = \$unused;
		} elsif(/^(\d+)\s+(?:$change_sign\s+)?(C\d{3})\s+(\w.*?)\s+([MC])\s*$/) { # composite
			push @{$segment->{elements}}, "$4$2";
			$in_composite = 1;
			$lasttextref = \$unused;
		} elsif(/^\s+(?:$change_sign\s+)?\d{4}\s+\w/ && $in_composite) { # skip composite copy
			$lasttextref = \$unused;
		} elsif(/^\s+Note:\s*(.*?)\s*$/) {
			$segment->{note} = $1;
			$lasttextref = \$segment->{note};
		} elsif(/^\s+(.*?)\s+$/ && $lasttextref) { # continuation of prev. line
			$$lasttextref .= " $1";
		} else {
			warn "Can't parse $fn line $.: <<$_>>\n" if $segment;
		}
	}
	$r->{delete $segment->{name}} = $segment if $segment && keys %$segment;
	close F;
	return $r;
}

sub load_composites_file
{
	my($fn, $r) = @_;
	$r ||= {};
	my $comp;
	my $lasttextref;
	my $change_sign = qr/[\+\*\#\|X\-]/;
	my $unused = '';
	open F, '<:bytes', $fn or die;
	while(<F>) {
		if(/^\s*$/) { # skip empty lines
			undef $lasttextref;
			next;
		}
		if(/^\xC4+/) { # horizontal ruler in win encoding
			$r->{delete $comp->{name}} = $comp if $comp && keys %$comp;
			$comp = {};
		} elsif(/^\s+(?:$change_sign )?(C\d{3}) {2}(\w.*?)\s*$/) { # "big" heading
			$comp->{name} = $1;
			$comp->{title} = $2;
			$lasttextref = \$comp->{title};
		} elsif(/^\s+Desc:\s*(.*?)\s*$/) {
			$comp->{function} = $1;
			$lasttextref = \$comp->{function};
		} elsif(/^(\d+)\s+(?:$change_sign\s+)?(\d{4})\s+(\w.*?)\s+([MC])\s+(a?n?(?:\.\.)?\d*)\s*$/) { # element
#			push @{$comp->{elements}}, [$1, $2, $3, $4];
			push @{$comp->{elements}}, "$4$2";
			$lasttextref = \$unused;
		} elsif(/^\s+Note:\s*(.*?)\s*$/) {
			$comp->{note} = $1;
			$lasttextref = \$comp->{note};
		} elsif(/^\s+(.*?)\s+$/ && $lasttextref) { # continuation of prev. line
			$$lasttextref .= " $1";
		} else {
			warn "Can't parse $fn line $.: <<$_>>\n" if $comp;
		}
	}
	$r->{delete $comp->{name}} = $comp if $comp && keys %$comp;
	close F;
	return $r;
}

sub load_elements_file
{
	my($fn, $r) = @_;
	$r ||= {};
	my $el;
	my $lasttextref;
	my $change_sign = qr/[\+\*\#\|X\-]/;
	my $unused = '';
	open F, '<:bytes', $fn or die;
	while(<F>) {
		if(/^\s*$/) { # skip empty lines
			undef $lasttextref;
			next;
		}
		if(/^\xC4+/) { # horizontal ruler in win encoding
			$r->{delete $el->{name}} = $el if $el && keys %$el;
			$el = {};
		} elsif(/^(?:$change_sign)*\s+(\d{4}) {2}(\w.*?)\s*(?:\[[A-Z]\]\s*)?$/) {
			$el->{name} = $1;
			$el->{title} = $2;
			$lasttextref = \$el->{title};
		} elsif(/^(?:$change_sign)?\s+Desc:\s*(.*?)\s*$/) {
			$el->{function} = $1;
			$lasttextref = \$el->{function};
		} elsif(/^(?:$change_sign)?\s+Repr:\s*(.*?)\s*$/) {
			$el->{format} = $1;
			undef $lasttextref;
		} elsif(/^(?:$change_sign)?\s+Note:\s*(.*?)\s*$/) {
			$el->{note} = $1;
			$lasttextref = \$el->{note};
		} elsif(/^\s+(.*?)\s+$/ && $lasttextref) { # continuation of prev. line
			$$lasttextref .= " $1";
		} else {
			warn "Can't parse $fn line $.: <<$_>>\n" if $el;
		}
	}
	$r->{delete $el->{name}} = $el if $el && keys %$el;
	close F;
	return $r;
}

sub load_codes_file
{
	my($fn, $r) = @_;
	$r ||= {};
	my $cv;
	my $lasttextref;
	my $change_sign = qr/[\+\*\#\|X\-]/;
	my $unused = '';
	my $mapkey;
	my $ind = 100;
	open F, '<:bytes', $fn or die "Can't read $fn: $!\n";
	while(<F>) {
		if(/^\s*$/) { # skip empty lines
			undef $lasttextref;
			next;
		}
		if(/^\xC4+/) { # horizontal ruler in win encoding
			$r->{delete $cv->{name}} = $cv if $cv && keys %$cv;
			$cv = {};
			undef $mapkey;
		} elsif(/^(?:$change_sign)*\s+(\d{4}) {2}(\w.*?)\s*(?:\[[A-Z]\]\s*)?$/) {
			$cv->{name} = $1;
			$cv->{title} = $2;
			$lasttextref = \$cv->{title};
		} elsif(/^(?:$change_sign)?\s+Desc:\s*(.*?)\s*$/) {
			$cv->{function} = $1;
			$lasttextref = \$cv->{function};
		} elsif(/^(?:$change_sign)?\s+Repr:\s*(.*?)\s*$/) {
			$cv->{format} = $1;
			$mapkey = format_to_regexp($1);
			undef $lasttextref;
		} elsif(/^(?:$change_sign)?\s+Note:\s*(.*?)\s*$/) {
			$cv->{note} = $1;
			$lasttextref = \$cv->{note};
		} elsif($mapkey && /^(?:$change_sign)*\s{1,$ind}($mapkey)\s+(?{$ind = pos()})(.*?)\s*$/) {
			$cv->{map}{$1} = [$2, ''];
			$lasttextref = \($cv->{map}{$1}[1]);
		} elsif(/^\s+(.*?)\s+$/ && $lasttextref) { # continuation of prev. line
			$$lasttextref .= ' ' if length($$lasttextref);
			$$lasttextref .= $1;
		} else {
			warn "Can't parse $fn line $.: <<$_>>\n" if $cv;
		}
	}
	$r->{delete $cv->{name}} = $cv if $cv && keys %$cv;
	close F;
	return $r;
}

sub format_to_regexp
{
	my($format, $is_optional) = @_;
	$format =~ m#^(a)?(n)?(\.\.)?(\d+)# or die "Can't parse format string <<$format>>\n";
	my $re = '[';
	$re .= 'A-Za-z' if $1;
	$re .= '0-9' if $2;
	$re .= ']';
	if($4) {
		if($3) {
			my $min = $is_optional ? 0 : 1;
			$re .= "{$min,$4}";
		} else {
			$re .= "{$4}";
		}
	}
#	warn "Format: $format, Regexp: $re\n";
	return qr/$re/;
}

sub load_annexb # service segments descriptions
{
	my($fn, $segs, $comps, $els) = @_;
	my $change_sign = qr/[\+\*\#\|X\-]/;
	$segs ||= {}; $comps ||= {}; $els ||= {};
	my($s, $c, $e);
	open F, '<:bytes', $fn or die;
	while(<F>) {
		last if /^ANNEX B/;
	}
	my($namepos, $remarkspos, $locremarks);
	my($lastrem, $lasttit);
	while(<F>) {
		last if /^ANNEX C/;
		if(/^\s*$/) {
			undef $lastrem;
			undef $lasttit;
			next;
		}
		$locremarks = find_remarks_col($_, $locremarks, $remarkspos) if $remarkspos;
		my $remtext = substr($_, $locremarks, length($_) - $locremarks, '') if $locremarks && length($_) > $locremarks; # chomp it off!
		if($remtext) {
			$remtext =~ s/^\s+//s;
			$remtext =~ s/\s+$//s;
#print "$_|$remtext\n";
		}
#else { print(('.' x $remarkspos)."v\n"); print 'X'.$_ }
#		print "remarkspos: $remarkspos, locremarks: $locremarks, remtext: $remtext\n";
		if(/^=====/) {
			undef $s;
			undef $remarkspos;
			undef $locremarks;
			undef $lastrem;
			undef $lasttit;
		} elsif(/^ ?____/) {
			undef $c;
			undef $locremarks;
			undef $lastrem;
			undef $lasttit;
		} elsif(/^\s*Segment: ([A-Z]{3})\, (.*?)\s*$/) {
			$s = { name => $1, title => $2, function => $remtext||'' };
			$lastrem = \$s->{function};
			$lasttit = \$s->{title};
			$segs->{$1} = $s;
		} elsif(/^ Ref\.\s+Repr\.\s+Name\s+(?{$remarkspos = pos()})Remarks/) { # skip
		} elsif(/^(?:$change_sign)? {0,2}([0-9]{4})\s+(a?n?(?:(?:\.\.)?\d+)?)\s+([MC])\s+(?{$namepos = pos()})(.*?)\s*$/) {
			$e = { name => $1, format => $2, title => $4, function => $remtext||'' };
			$lastrem = \$e->{function};
			$lasttit = \$e->{title};
			$els->{$1} = $e unless $els->{$1};
			push(@{$c ? $c->{elements} : $s->{elements}}, $3.$1);
		} elsif(/^ (S[0-9]{3})\s+([MC])\s+(.*?)\s*$/) { # composite
			$c = { name => $1, title => $3, title2 => "c$1", function => '' };
			$comps->{$1} = $c;
			push(@{$s->{elements}}, $2.$1);
		} elsif($namepos && $lasttit && /^ {$namepos}(\S.*?)\s*$/) {
			my $tail = $1;
			$$lasttit .= ' ' if $$lasttit =~ /\w$/s;
			$$lasttit .= $tail;
			if($remtext) {
				$$lastrem .= ' ' if $$lastrem =~ /\w$/s;
				$$lastrem .= $remtext;
			}
		} elsif($remtext && /^ +$/) {
			$$lastrem .= ' '.$remtext;
		} else {
			print "X$_\n";
		}
	}
	close F;
}

sub find_remarks_col
{
	my($text, $locpos, $remarkspos) = @_;
	return undef if length($text) < $remarkspos;
	my $maxdist = 6;
	local $_ = substr($text, $remarkspos - $maxdist, 2 * $maxdist);
	return $remarkspos - $maxdist + length($1) if /^(.* {2,})\S/;
	return $remarkspos - $maxdist + length($1) if /^(.*? )[A-Z]/;
	return $locpos;
}

1;
