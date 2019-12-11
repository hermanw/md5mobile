# md5mobile

**md5mobile** builds a formula in memory to decode md5 hash value back to the original mobile number. It supports Chinese mobile number in the format of 13812345678.

It has two projects:
- **mm** - ad-hoc client version. It reads a md5 hash file and decodes all hashes at once. It takes less than 100M memory while uses more cpu time.
- **mmserver** - http server version. It accepts htttp requests with md5 hash and responds with the decoded number. It responds in a few ms while takes more than 16G memory.
