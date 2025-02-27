#!/bin/bash

# Generic stdin <-> stdout autotester 
# Taylor Andrews

# Usage: see EOF

# finally, compare runlogged output against expected file
test_compare_files () {
    local file1="$1"
    local file2="$2"
    if [ -f "$file1" ] && [ -f "$file2" ]; then
        diff "$file1" "$file2"
        # if the files differ, we have a test failure
        if [ $? -ne 0 ]; then
            echo "Error: $file1 and $file2 differ"
            return 1
        fi
        # if they are the same, we have a test pass
        return 0
    else
        echo "Error: $file1 or $file2 does not exist"
        return 1
    fi

}

# run some input file against a binary and save the output
io_runlog_bin() {
    local infile="$1"
    local outfile="$2"
    local bincmd="$3"
    local binargs="$4"
    $bincmd $binargs < "$infile" &> "$outfile"  # Currently piping ALL output to the file (including STDERR)
}

test_fail(){
    local testname="$1"
    local infile="$2"
    local expectedfile="$3"
    local bincmd="$4"
    local binargs="$5"
    local outfile="$6"
    cat $outfile
    echo "(unexpected output above)"
    echo "Test $testname failed"
    echo "Params: infile=$infile, expectedfile=$expectedfile, bincmd=$bincmd, outfile=$outfile"
    return 1
}

# generic test harness runner 
test_run_helper() {
    local testname="$1"
    local infile="$2"
    local expectedfile="$3"
    local bincmd="$4"
    local binargs="$5"
    local outfile="$6"
    echo "Running test: $testname"
    # run the binary with the input file and a temporary output capture file to verify against expected output
    io_runlog_bin "$infile" "$outfile" "$bincmd" "$binargs"
    if [ $? -ne 0 ]; then
        echo "Error: run_bin failed"
        return 1
    fi
    if [ ! -f "$outfile" ]; then
        echo "Error: run_bin did not produce an output file"
        return 1
    fi
    # check the output against the expected output
    test_compare_files "$outfile" "$expectedfile"
    if [ $? -ne 0 ]; then
        echo "Test $testname binary produced output, but failed output check" 
        return 1
    else
        if [ -f "$outfile" ]; then # remove temporary output file
            rm "$outfile"
        fi
        echo "Test $testname passed!"
    fi
    return $?
}

test_run () {
    local testname="$1"
    local infile="$2"
    local expectedfile="$3"
    local bincmd="$4"
    local binargs="$5"
    local outfile="$testname-runlogged_out.tmp"
    local failpath=false
    if [ ! -f "$infile" ]; then
        echo "Error: input file '$infile' does not exist"
        failpath=true
    fi
    if [ -f "$outfile" ]; then
        echo "Warning: output file '$outfile' already exists"
        #failpath=true
    fi
    if [ ! -f "$expectedfile" ]; then
        echo "Error: expected content file '$expectedfile' does not exist"
        failpath=true
    fi
    if [ ! -f "$bincmd" ]; then
        echo "Error: binary '$bincmd' does not exist"
        failpath=true
    fi
    if [ "$failpath" = true ]; then
        test_fail "$testname" "$infile" "$expectedfile" "$bincmd" "$outfile"
        echo "Test $testname could not start due to path errors"
        return 1
    fi
    # Now that we've validated what we can up front, run the test
    test_run_helper "$testname" "$infile" "$expectedfile" "$bincmd" "$binargs" "$outfile"
    if [ $? -ne 0 ]; then # any failure, print what we can to help
        test_fail "$testname" "$infile" "$expectedfile" "$bincmd" "$outfile"
        return 1
    fi
    return 0
}

#
# Usage: 
#      add test_run calls to run tests here like:
#
#      test_run <testname> <inputfile> <expectedfile> <binpath> <binargs/argv>
# ex:  test_run "cs3013_p3_1" "p3_1_testin.txt" "p3_1_testout.txt" "./mmu" ""
# ...
#
#
# then run from project root directory like:
# cd PROJECT_ROOT; autotest.sh

# Make project -- use case specific 
make clean
make
if [ $? -ne 0 ]; then
    echo "Error: make failed"
    exit 1
fi

# Run tests -- use case specific -- renamed starting input and output files for test convention
test_run "p3_1-RR" "./test/p3_1-testin.txt" "./test/p3_1-expected.txt" "./mmu" ""
test_run "p3_2-RR" "./test/p3_2-testin.txt" "./test/p3_2-expected.txt" "./mmu" ""
# ...

# sanity check -- another copy of the very first input and output files
test_run "p3_o1-RR" "./test/sampleInput.txt" "./test/testOutput.txt" "./mmu" ""    
test_run "p3_o2-RR" "./test/sampleInput2.txt" "./test/testOutput2.txt" "./mmu" ""
# ...
