#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <iostream>

using namespace std;

#define ToBigEndian32(x) ((x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24))
#define ToBigEndian64(x) ((x >> 56) | ((x & 0x00FF000000000000) >> 40) | ((x & 0x0000FF0000000000) >> 24) | ((x & 0x000000FF00000000) >> 8) | ((x & 0x00000000FF000000) << 8) | ((x & 0x0000000000FF0000) << 24) | ((x & 0x000000000000FF00) << 40) |  (x << 56));

// self-defined data structure named "pako"
struct pako_header{
    uint32_t magic;
    uint32_t off_str;
    uint32_t off_data;
    uint32_t num_files;
};
// each file will has its own header
struct FILE_E{
    uint32_t off_fname;
    uint32_t fsize; // big endian
    uint32_t off_data;
    uint64_t checksum; // big endian
}__attribute__((packed));

int main(int argc, char *argv[]){

    int in = open(argv[1], O_RDONLY);
    if (in < 0) {
        cout << "Open failed\n";
        return -1;
    }
    
    // get pako header
    pako_header h;
    if (read(in, &h, sizeof(h)) <= 0) {
        cout << "Read failed\n";
        return -1;
    }
    cout << "The number of files packet in " << argv[1] << " : " << h.num_files << endl;

    // get file header
    const int num_f = h.num_files;
    FILE_E file_info[num_f];
    for (int i = 0; i < num_f; i++) {
        if (read(in, &file_info[i], sizeof(file_info[i])) <= 0) {
            cout << "Read failed\n";
            return -1;
        }
        file_info[i].fsize = ToBigEndian32(file_info[i].fsize);
        file_info[i].checksum = ToBigEndian64(file_info[i].checksum);
    }

    // get file content
    for (int i = 0; i < num_f; i++) {        
        // check checksum
        int32_t data_offset = h.off_data + file_info[i].off_data;
        uint64_t Csum = file_info[i].checksum;
        lseek(in, data_offset, SEEK_SET);
        for (int j = 0; j < file_info[i].fsize; j += 8) {
            uint64_t tmp = 0;
            if (read(in, &tmp, sizeof(tmp)) <= 0) {
                cout << "Read failed\n";
                return -1;
            }
            Csum ^= tmp;
        }        
        if (Csum != 0) {
            cout << "File No." << i+1 << " has incorrect checksum!" << endl;
            continue;
        }

        string filename = "";
        char rtmp;
        lseek(in, h.off_str + file_info[i].off_fname, SEEK_SET);
        while(true){
            read(in, &rtmp, 1);
            if (rtmp == '\0') break;
            filename += rtmp;
        }

        const int file_size = file_info[i].fsize;
        char file_content[file_size];
        lseek(in, h.off_data + file_info[i].off_data, SEEK_SET);
        read(in, &file_content, file_size);

        string slash = "/";
        // according to file name we get, open a new file to write
        int file_in = open((argv[2] + slash + filename).c_str(), O_WRONLY | O_CREAT, S_IRWXU);
        if (file_in <= 0) {
            cout << "Failed to open file No." << i+1 << endl << endl;
            return -1;
        }
        cout << file_size;
        // write the file content into the new file
        if (write(file_in, &file_content, file_size) <= 0) {
            cout << "Write failed\n";
            return -1;
        }
        close(file_in);

        cout << "File No." << i+1 << ":" << endl;
        cout << "\tfilename: " << filename << endl;
        cout << "\tfile size: " << file_size << endl;
        cout << endl;
    }
    close(in);

    return 0;
}

// test case
/*
NjUJNwKXOI
HZegjphmRF
EMHBXpiesN
nHIUQloxCv
XyKbwoJbLH
vGxVRVHynY
cywHKcpsTx
WhCKUUUflc
kyqxAnhuSF
eKEdGnFgOF
cadOnIOrxm
lAVoXflLct
cnjmHCXIWC
JovmZIbMXT
fMcZRPMGkm
aULQSNQBGO
DSSdkRNZiP
elssRXvOAT
checker
XCvmPtcIvp
tsbwCziPab
MHVOUSjAYJ
LpTlFedCwP
XsOaKimWTI
KxLtZZtOlM
kvSpFXrcQR
gPbYjmLoXa
oCFskqbVFr
VnGxDQewls
*/

/*
pqerlbHUYP
CHLurQueZO
JOHlLuQXMo
HdbQeJPRAX
tJWqYxLFuy
TFpFvjIBcX
JPMHfcniRu
JCUTOdgKpE
VWyONNDRCc
YTeyxJPnvx
ZBkAouSDhJ
nLEIWTMVZS
NFrSRCjQtm
UDQvEiTALc
TADJWYQzIu
csNXSMgAOP
CbqOzVLdmX
roIjHEfTfe
VCdCBvTmZu
XVmiVvduXC
uLCiqBXGfY
OXtEOYVoPu
UuJiiKbyLu
checker
PrDmEvaErW
rRLsSanmuN
DDdsbKfJhu
OWwofWMFCF
eMzUNDALJw
FkWBAEUMQW
nLaHGhcWcK
kWWITzpQJg
fdDbAVxCKT
vMUOQhmDOd
iVZrwlikcj
DvXHtnkljE
URIETREXOe
svlFNedLGp
BdRAfAPtrd
*/