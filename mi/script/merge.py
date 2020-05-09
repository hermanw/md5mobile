import csv

# load codemap from .csv files
codemap={}
for year in range(1980,2020):
    submap={}
    with open(str(year)+'.csv', 'r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            submap[row[0]] = row[1]
    codemap[year] = submap
    # print(str(year)+','+str(len(submap)))

# create mca.rs
file_mca = open('../src/mca.rs', "w")
file_mca.write('const FN_MCA: [fn()->Vec<usize>; '+ str(len(codemap)) +'] = [\n    ')
for year in range(len(codemap)):
    file_mca.write('get_mca_'+str(1980+year)+',')
file_mca.write('];\n')
file_mca.write('\npub fn get_mca(year : usize) -> Vec<usize> {')
file_mca.write('\n    let mut index = year as isize - 1980;')
file_mca.write('\n    if index < 0 {')
file_mca.write('\n        index = 0;')
file_mca.write('\n    } else if index > 39 {')
file_mca.write('\n        index = 39;')
file_mca.write('\n    }')
file_mca.write('\n    FN_MCA[index as usize]()')
file_mca.write('\n}\n\n')

# merge code map for each year
for year in range(1980,2020):
    file_mca.write('fn get_mca_'+str(year)+'() -> Vec<usize> {')
    file_mca.write('\n    vec![')
    newmap={}
    for to_merge in range(year,2020):
        newmap={**newmap,**codemap[to_merge]}
    # print(str(year)+','+str(len(newmap)))    
    for code in newmap.keys():
        file_mca.write(code+',')
    file_mca.write(']\n}\n\n')

# close mca.rs
file_mca.close()
