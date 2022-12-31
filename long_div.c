#if 0

function invoke {
	n=$1
	d=$2
	
	# Add a line below to call your program.  Examples:
	#    ./program-taking-arguments $n $d
	#    { echo $n; echo $d; } | ./program-reading-stdin
}

# This "script" can be used in two ways:
#  * As a bash script.
#    - Without arguments, it runs the test suite, using the embedded C program
#      to compute the expected results.
#    - With two arguments, it runs the embedded C program with the arguments given.
#  * As a C program.  Compile it, then run it, passing the numerator
#    and denominator as arguments.  This will print the expected output.

if [ $# -ne 0 -a $# -ne 2 ] ; then
	echo "Usage: $0 [<numerator> <denominator>]" >&1
	exit 1
fi

control="$(mktemp)"
gcc -x c "$0" -o "$control" || exit 1

# Colorized pass/fail strings
PASS="$(printf '\e[32mPASS\e[0m')"
FAIL="$(printf '\e[1;31mFAIL\e[0m')"

function testcase {
	n=$1
	d=$2
	
	# One-liner for deleting trailing blank lines
	# acquired from http://www.suwald.com/linux-gnu/sed-howto.html :
	#     sed -e :a -e '/^\n*$/{$d;N;};/\n$/ba'
	
	if cmp <("$control" $n $d        | sed 's/\s*$//' | sed -e :a -e '/^\n*$/{$d;N;};/\n$/ba') \
	       <({ invoke $n $d; echo; } | sed 's/\s*$//' | sed -e :a -e '/^\n*$/{$d;N;};/\n$/ba') \
	       &>/dev/null; then
		echo "$PASS $n ÷ $d"
		(( pass++ ))
	else
		echo "$FAIL $n ÷ $d"
	fi
	
	(( total++ ))
}

if [ $# -eq 2 ] ; then
	"$control" "$1" "$2"
else
	pass=0
	total=0
	expected=35
	
	testcase 1234 56
	testcase 1002012 12
	testcase 1 1
	testcase 0 1
	testcase 10 1
	testcase 100 1
	testcase 300 1
	testcase 100 3
	testcase 100 10
	testcase 100 100
	testcase 100 1000
	testcase 1 7
	testcase 10 7
	testcase 100 7
	testcase 1000 7
	testcase 10000 7
	testcase 100000 7
	testcase 10000000000 7
	testcase 100000000000000000000000000000000000000000000000000000000000000000000000 7
	testcase 1 13
	testcase 10 13
	testcase 100 13
	testcase 1000 13
	testcase 100000000000000000000000000000000000000000000000000000000000000000000000 13
	testcase 123123123 123
	testcase 123000123000123 123
	testcase 124000124000124000124000 123
	testcase 1 9999999
	testcase 1000000 9999999
	testcase 10000000000000 9999999
	testcase 100000000000000 9999999
	testcase 100000000000000000000 9999999
	testcase 100000000000000000000000000000000000000000000000000000000000000000000000 9999999
	testcase 999999999999999999999999999999999999999999999999999999999999999999999999 9999999
	testcase 999999999999999999999999999999999999999999999999999999999999999999999999 1
	
	echo
	echo "Score: $pass / $total"
	if [ $pass -eq $expected ] ; then
		echo "All tests passed!"
	elif [ $pass -eq 0 ] ; then
		echo "No tests passed."
	else
		echo "Not all tests passed."
	fi
fi

rm "$control"

exit

#else

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char canvas[999][99];

const char *numerator_str, *denominator_str;
int denominator;

const char *numerator_cursor;
char       *quotient_cursor;

static void divide(int row, int col, int difference);
static void printCanvas(void);
static bool printRow(int row);

static int digitCount(int n);

int main(int argc, char *argv[])
{
	assert(argc == 3);
	numerator_str = argv[1];
	denominator_str = argv[2];
	
	assert(strlen(numerator_str) > 0);
	assert(strlen(denominator_str) > 0);
	assert(atoi(denominator_str) > 0);
	assert((int) strlen(denominator_str) == digitCount(atoi(denominator_str)));
	
	denominator = atoi(denominator_str);
	numerator_cursor = numerator_str;
	quotient_cursor = canvas[0] + strlen(denominator_str) + 1;
	
	{
		char *quotient_start = quotient_cursor;
		int   numerator_column = strlen(denominator_str) + 1;
	
		memset(canvas[0], ' ', numerator_column);
		memset(canvas[0] + numerator_column, '0', strlen(numerator_str));
		memset(canvas[1], ' ', numerator_column);
		memset(canvas[1] + numerator_column, '-', strlen(numerator_str));
		sprintf(canvas[2], "%s|%s", denominator_str, numerator_str);
	
		/* Start the recursion by "bringing down" digits on top of the numerator. */
		divide(2, numerator_column, 0);
	
		/* Remove leading zeros in the quotient. */
		{
			char *s = quotient_start;
			char *e = s + strlen(numerator_str);
		
			for (; s < e-1 && *s == '0'; s++)
				*s = ' ';
		}
	}
	
	printCanvas();
	
	return 0;
}

/*
 * row:        Row of the difference, and row onto which to bring down digits.
 * col:        Column following the difference and preceeding where to bring down more digits.
 * difference: Result of previous subtraction.
 */
static void divide(int row, int col, int difference)
{
	int line_start;
	int brought_down;
	
	assert(difference < denominator);
	
	if (difference != 0) {
		/* Print the difference */
		line_start = col - digitCount(difference);
		sprintf(canvas[row] + line_start, "%d", difference);
	} else {
		/* Skip zeros in the numerator. */
		for (; *numerator_cursor == '0'; numerator_cursor++, quotient_cursor++, col++)
			{}
		line_start = col;
	}
	
	brought_down = difference;
	
	/* Bring down digits until we have a big enough number.
	 * Add digits to the quotient accordingly. */
	while (brought_down < denominator) {
		if (*numerator_cursor == '\0')
			return; /* No more digits to bring down. */
		
		canvas[row][col++] = *numerator_cursor;
		
		brought_down *= 10;
		brought_down += *numerator_cursor++ - '0';
		*quotient_cursor++ = '0' + brought_down / denominator;
	}
	
	/* Print the subtrahend and draw the line,
	 * then proceed to the next iteration. */
	{
		int q = brought_down / denominator;
		int m = q * denominator;
		assert(q >= 1 && q <= 9);
	
		sprintf(canvas[row + 1] + col - digitCount(m), "%d", m);
		memset(canvas[row + 2] + line_start, '-', col - line_start);
		divide(row + 3, col, brought_down - m);
	}
}

static void printCanvas(void)
{
	int row;
	
	for (row = 0; row < 999 && printRow(row); row++)
		{}
}

static bool printRow(int row)
{
	int col;
	
	for (col = 0; col < 99 - 1; col++) {
		if (canvas[row][col] != '\0') {
			while (col > 0)
				canvas[row][--col] = ' ';
			puts(canvas[row]);
			return true;
		}
	}
	
	return false;
}

/* Note: digitCount(0) == 0 (as in "") rather than 1 (as in "0"). */
static int digitCount(int n)
{
	int i;
	for (i = 0; n > 0; i++, n /= 10)
		{}
	return i;
}

#endif
