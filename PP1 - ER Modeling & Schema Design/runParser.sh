files="$(ls ebay_data)"
args=""
for file in $files
do
    args="${args} ebay_data/${file}"
done

python skeleton_parser.py $args