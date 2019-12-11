# mmserver

**md5mobile** builds a formula in memory to decode md5 hash value back to the original mobile number. It supports Chinese mobile number in the format of 13812345678.
**mmserver** is the server version. It start a http server. You can send a request to http://localhost:8000/[hash] and get back the decoded mobile number.

## Usage:
mmserver [port]
-- default port is 8000