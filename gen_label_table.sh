#! /bin/sh

# Pre-compute label table

label_count_max=1000
label_buf_siz=6  # '.L'+N digits+NUL
label_seq_max=$((label_count_max-1))

echo "// This file was automatically generated by $0"

echo '#pragma once'
echo "#define label_count_max $label_count_max"
echo "#define label_buf_siz $label_buf_siz"
echo "static char labels[][label_buf_siz] = {"
for i in $(seq 0 $label_seq_max) ; do
    echo "    \".L$i\","
done
echo "};"

echo "// End of autogenerated text"
