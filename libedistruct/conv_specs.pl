#!/usr/bin/perl
use utf8;
use strict;
use warnings;
use Data::Dumper;
use FindBin;
use lib $FindBin::Bin;
use UneceParser;

my $z = {};
UneceParser::load_codes_file('uncl/UNCL.99A', $z);
UneceParser::load_codes_file('unsl/UNSL.99A', $z);
#print Dumper($z);

my $s = {};
my $c = {};
my $e = {};
UneceParser::load_segments_file('edsd/EDSD.99A', $s);
UneceParser::load_composites_file('edcd/EDCD.99A', $c);
UneceParser::load_elements_file('eded/EDED.99A', $e);
UneceParser::load_annexb('part4/D422.TXT', $s, $c, $e);
#print "SEGMENTS: ".Dumper($s);

# SYNTAX AND SERVICE REPORT MESSAGE (CONTROL)
# http://www.unece.org/trade/untdid/download/r1186.pdf
$s->{UCI} = {
	title => 'Interchange response',
	function => 'To identify the subject interchange, to indicate interchange receipt, to indicate acknowledgement or rejection (action taken) of the UNA, UNB and UNZ segments, and to identify any error related to these segments. Depending on the action code, it may also indicate the action taken on the functional groups and messages within that interchange.',
	elements => [
#		'M0020', # it is mandatory in documentation!!! but in "translated documentation" it is absend.
		'MS002',
		'MS003',
		'M0020', # in unice docs it is absend, but present in "translated documentation"...
		'M0083',
		'C0085',
		'C0013',
		'CS011'
	],
#	note => 'This segment also occurs in the following versions of this standard: 30000, 40000, 40100',
};
$e->{'0013'} = {
	title => 'Service segment tag, coded',
	function => 'Code identifying a service segment.',
	format => 'a3',
#	note => 'This table also occurs in the following versions of this standard: 99A, 99B, 00A, 00B, 01A, 01B, 01C, 02A, 02B, 03A, 03B',
}; # codes are there... strange...
$e->{'0083'} = {
	title => 'Action, coded',
	function => 'A code indicating acknowledgement, or rejection (the action taken) of a subject interchange, or part of the subject interchange.',
	format => 'an..3',
#	note => 'This table also occurs in the following versions of this standard: 40000, 40001, 40002, 40003, 40004, 40005, 40006, 40007, 40100, 40101, 40102, 94A, 94B, 95B, 96A, 96B, 97A, 99A, 99B, 00A, 00B, 01A, 01B, 01C, 02A, 02B, 03A, 03B, D93A',
}; # codes are there... strange...
$e->{'0085'} = {
	title => 'Syntax error, coded',
	function => 'A code indicating the syntax error detected.',
	format => 'an..3',
#	note => 'This table also occurs in the following versions of this standard: 40000, 40001, 40002, 40003, 40004, 40005, 40006, 40007, 40100, 40101, 40102, 94A, 94B, 95B, 96A, 96B, 97A, 99A, 99B, 00A, 00B, 01A, 01B, 01C, 02A, 02B, 03A, 03B, D93A',
}; # codes are there... strange...
$c->{S011} = {
	title => 'DATA ELEMENT IDENTIFICATION',
	function => 'Identification of the position for an erroneous data element. This can be the position of a simple or composite data element in the definition of a segment or a component data element in the definition a composite data element.',
	elements => [ 'M0098', 'C0104' ],
};
$e->{'0098'} = {
	title => 'Erroneous data element position in segment',
	function => 'The numerical count position of the simple or composite data element in error. The segment code and each following simple or composite data element defined in the segment description shall cause the count to be incremented. The segment tag has position number 1.',
	format => 'n..3',
};
$e->{'0104'} = {
	title => 'Erroneous component data element position',
	function => 'The numerical count position of the component data element in error. Each component data element position defined in the composite data element description shall cause the count to be incremented. The count starts at 1.',
	format => 'n..3',
};
$s->{UCM} = {
	title => 'MESSAGE RESPONSE',
	function => 'To identify a message in the subject interchange, and to indicate that message\'s acknowledgement or rejection (action taken), and to identify any error related to the UNH and UNT segments.',
	elements => [
		'MS012', # not in pdf, in doc,
#		'M0062', # - definetely, error in doc,
		'MS009',
#		'M0083', # absent in doc
#		'C0085', # absent in doc
#		'C0013', # absent in doc
#		'CS011', # absent in doc
	],
};
$c->{'S012'} = {
	title => 'cS012',
	function => '',
	elements => [
		'M0062',
		'C0068',
	],
};
$s->{UCS} = { # only in pdf
	title => 'SEGMENT ERROR INDICATION',
	function => 'To identify either a segment containing an error or a missing segment, and to identify any error related to the complete segment.',
	elements => [ 'M0096', 'C0085' ],
};
$e->{'0096'} = {
	title => 'SEGMENT POSITION IN MESSAGE',
	function => '???',
	format => 'n..6',
};
$s->{UCF} = { # only in pdf
	title => 'FUNCTIONAL GROUP RESPONSE',
	function => 'To identify a functional group in the subject interchange and to indicate acknowledgement or rejection (action taken) of the UNG and UNE segments, and to identify any error related to these segments. Depending on the action code, it may also indicate the action taken on the messages within that functional group.',
	elements => [ 'M0048', 'MS006', 'MS007', 'M0083', 'C0085', 'C0013', 'CS011' ],
};
$s->{UCX} = { # from doc ??
	title => 'THE LEVEL OF MESSAGE AND ERROR ON MESSAGE LEVEL',
	function => 'Whether there is an EDIFACT, qualifier or code error in the segment is advised.',
	elements => [ 'M0083', 'C0085' ],
};
$s->{UCR} = {
	title => 'ERRONEUS SEGMENT IN MESSAGE',
	function => 'A segment error is advised.',
	elements => [ 'M0096', 'C0085' ],
};
$s->{UCD} = {
	title => 'DATA ELEMENT ERROR INDICATION',
	function => 'The error in a data element is advised.',
	elements => [ 'MS011', 'C0085' ],
};

if(0) {
	open F, ">", "segs.c" or die;
	print F make_c_defs_segments($s, 'segment');
	close F or die;

	open F, ">", "comps.c" or die;
	print F make_c_defs_segments($c, 'composite');
	close F or die;

	open F, ">", "elems.c" or die;
	print F make_c_defs_elements($e, 'element', $z);
	close F or die;

	open F, ">", "coded.c" or die;
	print F make_c_defs_coded($z);
	close F or die;
} else {
	open F, ">", "segs.xml" or die;
	print F make_xml_defs_segments($s, 'segment');
	close F or die;

	open F, ">", "comps.xml" or die;
	print F make_xml_defs_segments($c, 'composite');
	close F or die;

	open F, ">", "elems.xml" or die;
	print F make_xml_defs_elements($e, 'element', $z);
	close F or die;

	open F, ">", "coded.xml" or die;
	print F make_xml_defs_coded($z);
	close F or die;
}

#################### C code generation ####################

sub make_c_defs_segments
{
	my($s, $namepart) = @_;
	my $struct_name = "edistruct_${namepart}";
	my $var_name = "${namepart}s";
	my $structs = "static struct ${struct_name} ${var_name}[] = {\n";
	my $els = '';
	foreach my $seg(sort keys %$s) {
		my $title1 = $s->{$seg}{title};
		my $title2 = make_title2($title1);
		my $function = $s->{$seg}{function};
		my $children = "${namepart}_${seg}_children";
		$structs .= qq#\t{ "$seg", "$title1", "$title2", "$function", $children },\n#;
		$els .= qq#static const char * ${children}[] = {#;
		foreach my $c(@{ $s->{$seg}{elements} }) {
			$els .= qq# "$c",#;
		}
		$els .= qq# NULL };\n#;
	}
	$structs .= "};\n";
	return join("\n\n", "/* Autogenerated file */", "#include \"libedistruct.h\"\n#include <string.h>", $els, $structs, make_find_name_function($struct_name, $var_name));
}

sub make_c_defs_elements
{
	my($s, $namepart, $codes) = @_;
	my $struct_name = "edistruct_${namepart}";
	my $var_name = "${namepart}s";
	my $structs = "static struct ${struct_name} ${var_name}[] = {\n";
	foreach my $el(sort keys %$s) {
		my $title1 = $s->{$el}{title};
		my $title2 = make_title2($title1);
		my $function = $s->{$el}{function};
		my $format = $s->{$el}{format};
		my $is_coded = $codes->{$el} ? 1 : 0;
		$structs .= qq#\t{ "$el", "$title1", "$title2", "$function", "$format", $is_coded },\n#;
	}
	$structs .= "};\n";
	return join("\n\n",
		"/* Autogenerated file */",
		"#include \"libedistruct.h\"\n#include <string.h>",
		$structs,
		make_find_name_function($struct_name, $var_name));
}

sub make_title2
{
	my($title1) = @_;
	my $title2 = lc $title1;
	$title2 =~ s/\'//sg;
	$title2 =~ s/^\W+//s;
	$title2 =~ s/\W+$//s;
	$title2 =~ s/,\s+coded\W*$//;
	$title2 =~ s#\W+#_#sg;
	return $title2;
}

sub make_find_name_function
{
	my($struct, $var) = @_;
return << "***";
const struct ${struct} * find_${struct}(const char * name)
{
	size_t begin = 0;
	size_t end = sizeof(${var})/sizeof(${var}[0]);
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, ${var}[cur].name);
		if(cmp == 0) return & ${var}[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}
***
}

sub make_c_defs_coded
{
	my($z) = @_;
	my $vals = '';
	my $structs = "static struct coded_values coded_values_table[] = {\n";
	foreach my $el(sort keys %$z) {
		my $x = $z->{$el}{map};
		my $vals_name = "vals_$el";
		$vals .= "static edistruct_coded_t ${vals_name}[] = {\n";
		foreach my $val(sort keys %$x) {
			my($title1, $function) = @{ $x->{$val} };
			my $title2 = make_title2($title1);
			$function = escape_c($function);
			$title1 = escape_c($title1);
			$vals .= qq#\t{ "$val", "$title1", "$title2", "$function" },\n#;
		}
		$vals .= "};\n\n";
		my $vals_size = scalar keys %$x;
		$structs .= qq#\t{ "$el", $vals_name, $vals_size },\n#;
	}
	$structs .= "};\n";
	return join("\n\n",
		"/* Autogenerated file */",
		"#include \"libedistruct.h\"\n#include <string.h>",
		"struct coded_values { const char * el; const edistruct_coded_t * vals; size_t nvals; };\n",
		$vals,
		$structs,
		make_find_coded_value_function()
	);
}

sub escape_c
{
	local($_) = @_;
	s#([\'\"\\])#\\$1#sg;
	return $_;
}

sub make_find_coded_value_function
{
	return << '***';
static const struct edistruct_coded * find_actual_value(const struct coded_values * tab, const char * name)
{
	size_t begin = 0;
	size_t end = tab->nvals;
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(name, tab->vals[cur].name);
		if(cmp == 0) return &tab->vals[cur];
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}

const struct edistruct_coded * find_coded_value(const char * elem, const char * name)
{
	size_t begin = 0;
	size_t end = sizeof(coded_values_table)/sizeof(coded_values_table[0]);
	for(;;) {
		size_t cur = (begin + end) / 2;
		int cmp = strcmp(elem, coded_values_table[cur].el);
		if(cmp == 0) return find_actual_value(&coded_values_table[cur], name);
		if(cmp < 0) end = cur; else begin = cur + 1;
		if(end - begin < 1) return NULL;
	}
}
***
}



#################### XML generation ####################


sub escape_xml
{
	local($_) = @_;
	s#\&#\&amp;#sg;
	s#\<#\&lt;#sg;
	s#\>#\&gt;#sg;
	s#\"#\&quot;#sg;
	return $_;
}
sub escape_html
{
	return escape_xml(@_);
}

sub mk_element
{
	my $n = shift;
	my $attrs = {};
	if(ref($_[0])) {
		$attrs = shift;
	}
	my $r = "<$n";
	$r .= " $_=\"$attrs->{$_}\"" foreach(keys %$attrs);
	if(@_) {
		$r .= ">";
		$r .= $_ foreach @_;
		$r .= "</$n>";
	} else {
		$r .= " />";
	}
	return $r;
}

sub make_xml_defs_segments
{
	my($s, $namepart) = @_;
	my @a;
	foreach my $seg(sort keys %$s) {
		my $title1 = $s->{$seg}{title};
		my $title2 = make_title2($title1);
		my $function = $s->{$seg}{function};
		my @children;
		foreach my $c(@{ $s->{$seg}{elements} }) {
			push @children, mk_element('child', $c);
		}
		my $e = mk_element($namepart, { code => $seg },
			mk_element('title1', $title1),
			mk_element('title2', $title2),
			mk_element('function', $function),
			mk_element('children', @children),
		);
		push @a, $e."\n";
	}
	return mk_element($namepart.'s', @a);
}

sub make_xml_defs_elements
{
	my($s, $namepart, $codes) = @_;
	my @a;
	foreach my $el(sort keys %$s) {
		my $title1 = $s->{$el}{title};
		my $title2 = make_title2($title1);
		my $function = $s->{$el}{function};
		my $format = $s->{$el}{format};
		my $is_coded = $codes->{$el} ? 1 : 0;
		my $e = mk_element($namepart, { code => $el },
			mk_element('title1', $title1),
			mk_element('title2', $title2),
			mk_element('function', $function),
			mk_element('format', $format),
			mk_element('is_coded', $is_coded),
		);
		push @a, $e."\n";
	}
	return mk_element($namepart.'s', @a);
}

sub make_xml_defs_coded
{
	my($z) = @_;
	my @a;
	foreach my $el(sort keys %$z) {
		my $x = $z->{$el}{map};
		my @b;
		foreach my $val(sort keys %$x) {
			my($title1, $function) = @{ $x->{$val} };
			my $title2 = make_title2($title1);
			$function = escape_xml($function);
			$title1 = escape_xml($title1);
			my $e = mk_element('value', { code => $val },
				mk_element('title1', $title1),
				mk_element('title2', $title2),
				mk_element('function', $function),
			);
			push @b, $e;
		}
		push @a, mk_element('coded_element', { name => $el }, @b)."\n";
	}
	return mk_element('coded_values', @a);
}
