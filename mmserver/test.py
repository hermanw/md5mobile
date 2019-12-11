import urllib.request
import hashlib
import random

PREFIX_LIST = [
    186, 158, 135, 159,
    136, 150, 137, 138,
    187, 151, 182, 152,
    139, 183, 188, 134,
    185, 189, 180, 157,
    155, 156, 131, 132,
    133, 130, 181, 176,
    177, 153, 184, 178,
    173, 147, 175, 199,
    166, 170, 198, 171,
    191, 145, 165, 172
]

for prefix in PREFIX_LIST:
    mobile = str(prefix*100000000 + random.randrange(100000000))
    hash = hashlib.md5(mobile.encode('utf-8')).hexdigest()
    print(mobile, urllib.request.urlopen("http://localhost:8000/"+hash).read())