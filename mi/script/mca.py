from urllib.request import urlopen
from html.parser import HTMLParser
from bs4 import BeautifulSoup

# function
def getcodemap(url):
    codemap = {}
    html = urlopen(url).read()
    soup = BeautifulSoup(html, 'html.parser')
    for tr in soup.find_all('tr'):
        code = ""
        area = ""
        for child in tr.children:
            # get first two td elements
            if child.name == 'td':
                s = ''
                for text in child.stripped_strings:
                    s = text
                    break
                if len(s) > 0:
                    if len(code) == 0:
                        code = s
                    elif len(area) == 0:
                        area = s
                    else:
                        break
        if len(code) == 6 and code.isdigit():
            codemap[code] = area

    return codemap

# main code
urllist = []
codepagelist = []
host = "http://www.mca.gov.cn" 
root = "/article/sj/xzqh/1980/"

for page in ["","?2","?3"]:
    url = host+root+page
    html = urlopen(url).read()
    soup = BeautifulSoup(html, 'html.parser')
    for a in soup.find_all('a'):
        if 'class' in a.attrs.keys():
            if a['class'][0] == 'artitlelist':
                urllist.append((a['title'],a['href']))

# remove the last two
urllist.pop()
urllist.pop()

# for title,url in urllist:
#     print(title + ":" + url)

# parse 2019-2011
for page in range(9):
    url = host+urllist[page][1]
    # print(url)
    html = urlopen(url).read()
    soup = BeautifulSoup(html, 'html.parser')
    div = soup.find(id="zoom")
    codepage = div.find_all('a')[0]['href']
    codepagelist.append((urllist[page][0],codepage))

# parse 2010-1980
for page in range(9,len(urllist)):
    url = host+urllist[page][1]
    html = urlopen(url).read()
    s = str(html,'utf-8')
    t = "window.location.href=\""
    start = s.find(t)+len(t)
    end = s.find("\"",start)
    codepagelist.append((urllist[page][0],s[start:end]))

print(len(codepagelist))

# parse each page and merge the code map
merged = {}
# codepagelist.append(('test','http://www.mca.gov.cn/article/sj/xzqh/1980/201705/201705311652.html'))
for codepage in reversed(codepagelist):
    print(codepage[0])
    print(codepage[1])
    codemap = getcodemap(codepage[1])
    print(len(codemap))
    file = open(codepage[0]+'.csv', "w")
    for code in codemap.keys():
        file.write(code+','+codemap[code]+'\n')
        merged[code] = codemap[code]
    file.close()

# wrtie to file merged.csv
print("merged: " + str(len(merged)))
file = open('merged.csv', "w")
for code in merged.keys():
    file.write(code+','+merged[code]+'\n')
file.close()
