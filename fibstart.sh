echo "setenv NOECHO 1"
setenv counter 0
setenv end 5
setenv last0 1
setenv last1 1
echo Shell path is: $0
$0 fib.sh
echo final result was $?
