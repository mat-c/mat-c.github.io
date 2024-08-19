this is a exception library for C I did for fun.

It use the famous setjmp/longjmp function (in fact I use the posix 
sigsetjmp/siglongjmp in order to make it work with signal).

I tried to make the syntax looking natural for C.
The section begin with a
"BEGIN_EXP {"
then code that could throw exception could be written. There is a small
limitation. The code should reach the end of the block (so shouldn't do
any return, break, continue, goto, ... outside of the block).

the block is ended with a 
"}"
It should be folowed by a section for code that catch interrupt.
It start with
"SWITCH_EXP(exp_val) {"
exp_val is an integer containing the exception code.
To define handler for exception case is used (like in a normal switch).

The end of this block is finish with
"} END_EXP;

Exception could be throw with the function throw_exp(value).

The value is an interger that is different of 0.


This code support "recursive" exception, and a default handler that print a
backtrace if the exception is not catched.

Some examples can be found on test.c

== Limitation ==
This require gcc (for local label, TLS) and glibc for backtrace support.

The backtrace support is very limited (and don't work if the exception come
from an interrupt).

Documentation is missing

Some internal variable should be hidden in order to avoid symbol clash
