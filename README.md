
## Example Run
Example run on a c5n.18xlarge instance:
```
./s3benchmark  --threads-min 108 --threads-max 216 --object-name "benchmark/largefile.bin" --samples $((144*3)) --payloads-min $((1024*1024*6)) --payloads-max $((1024*1024*24))
```
