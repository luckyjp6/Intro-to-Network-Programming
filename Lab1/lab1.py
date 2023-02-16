import os
import base64

# record packets and store the data in "result.pcap"
result = os.popen("tshark -r result.pcap -T fields -e tcp.srcport -e ip.ttl -e data")
ret = result.read()

# key/value: ttl/num -> indicate the number of packets that have the same ttl with the key
ttl_dic = {} 
# key/value: ttl/id -> indicate one of the packet's id which have the same ttl with the key
index_dic = {} 

data_split = ret.split("\n") # split each packet
data_wanted = ''
for i, value in enumerate(data_split):
    # split the attributes of the packet
    column = value.split('\t', 2)
    # only analysis packet from port == 10001 (source port)
    if column[0] != '10001': continue
    if len(column[2]) <= 0 : continue
    # record the current packet's ttl
    ttl = int(column[1])
    look_up = ttl_dic.get(ttl)
    if look_up == None:
        ttl_dic[ttl] = 1
    else:
        ttl_dic[ttl] += 1
    index_dic[ttl] = i

# get the unique ttl
unique_ttl = [key for key, value in ttl_dic.items() if value == 1][0]
# get the packet id owns the unique ttl
data_id = index_dic[unique_ttl]
# get the wanted packet
line_wanted = data_split[data_id].split('\t')
data_wanted = line_wanted[2]

# print the wanted packet
try:
    bytes_data = bytes.fromhex(data_wanted)
    ascii_data = bytes_data.decode()
    decode_data = base64.b64decode(ascii_data)
    print(decode_data)
except:
    print("Didn't catch the magic packet")
