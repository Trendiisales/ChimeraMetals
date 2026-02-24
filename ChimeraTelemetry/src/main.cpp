#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>
#include <atomic>
#include <cstdint>
#include <cstring>

#pragma comment(lib,"ws2_32.lib")

// ═══════════════════════════════════════════════════════════════════════════
// SHARED MEMORY STRUCTURE (must match TelemetrySnapshot.hpp)
// ═══════════════════════════════════════════════════════════════════════════

struct TelemetrySnapshot
{
    std::atomic<uint64_t> sequence;

    double xau_bid;
    double xau_ask;
    double xag_bid;
    double xag_ask;

    double xau_vwap;
    double xau_ema_fast;
    double xau_ema_slow;

    double xag_vwap;
    double xag_ema_fast;
    double xag_ema_slow;

    double hft_pnl;
    double strategy_pnl;

    double fix_rtt_last;
    double fix_rtt_p50;
    double fix_rtt_p95;

    double vps_latency;

    char hft_regime[32];
    char strategy_regime[32];

    char hft_trigger[32];
    char strategy_trigger[32];
};

// ═══════════════════════════════════════════════════════════════════════════
// SHARED MEMORY READER
// ═══════════════════════════════════════════════════════════════════════════

static TelemetrySnapshot* g_snapshot = nullptr;

bool SnapshotInit()
{
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\ChimeraTelemetrySharedMemory");
    if (!hMap) return false;

    g_snapshot = (TelemetrySnapshot*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(TelemetrySnapshot));
    return g_snapshot != nullptr;
}

bool SnapshotRead(TelemetrySnapshot& out)
{
    if (!g_snapshot) return false;

    uint64_t seq1;
    uint64_t seq2;

    do
    {
        seq1 = g_snapshot->sequence.load(std::memory_order_acquire);
        std::memcpy(&out, g_snapshot, sizeof(TelemetrySnapshot));
        seq2 = g_snapshot->sequence.load(std::memory_order_acquire);

    } while (seq1 != seq2);

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// WEBSOCKET SERVER
// ═══════════════════════════════════════════════════════════════════════════

static const char* MUTEX_NAME="Global\\ChimeraTelemetrySingleton";
static const char* PORT="8080";

bool send_all(SOCKET s,const char* data,int len){
    int total=0;
    while(total<len){
        int sent=send(s,data+total,len-total,0);
        if(sent<=0) return false;
        total+=sent;
    }
    return true;
}

std::string base64_encode(const unsigned char* data,size_t len){
    static const char table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len+2)/3)*4);
    for(size_t i=0;i<len;i+=3){
        unsigned int val=data[i]<<16;
        if(i+1<len) val|=data[i+1]<<8;
        if(i+2<len) val|=data[i+2];
        out.push_back(table[(val>>18)&0x3F]);
        out.push_back(table[(val>>12)&0x3F]);
        out.push_back((i+1<len)?table[(val>>6)&0x3F]:'=');
        out.push_back((i+2<len)?table[val&0x3F]:'=');
    }
    return out;
}

std::string generate_accept_key(const std::string& key){
    std::string combined=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()),combined.size(),hash);
    return base64_encode(hash,SHA_DIGEST_LENGTH);
}

std::string load_dashboard(){
    // Try multiple paths for dashboard
    const char* paths[] = {
        "dashboard\\index.html",
        "..\\dashboard\\index.html",
        "C:\\ChimeraMetals\\dashboard\\index.html"
    };
    
    for(const char* path : paths){
        std::ifstream file(path,std::ios::binary);
        if(file.is_open()){
            std::ostringstream ss;
            ss<<file.rdbuf();
            return ss.str();
        }
    }
    
    return "<h1>Dashboard Missing</h1><p>Searched multiple paths but couldn't find index.html</p>";
}

void send_text_frame(SOCKET s,const std::string& msg){
    std::vector<unsigned char> frame;
    frame.push_back(0x81);

    size_t len=msg.size();

    if(len<=125){
        frame.push_back((unsigned char)len);
    }else if(len<=65535){
        frame.push_back(126);
        frame.push_back((len>>8)&0xFF);
        frame.push_back(len&0xFF);
    }else{
        frame.push_back(127);
        for(int i=7;i>=0;--i)
            frame.push_back((len>>(8*i))&0xFF);
    }

    frame.insert(frame.end(),msg.begin(),msg.end());
    send_all(s,(char*)frame.data(),frame.size());
}

void websocket_loop(SOCKET s){
    TelemetrySnapshot snap;
    char json_buffer[1024];
    
    while(true){
        // Read from shared memory
        bool hasData = SnapshotRead(snap);
        
        if(hasData){
            // Build JSON from REAL data
            snprintf(json_buffer, sizeof(json_buffer),
                R"({"xau_bid":%.2f,"xau_ask":%.2f,"xag_bid":%.2f,"xag_ask":%.2f,"hft_pnl":%.2f,"strategy_pnl":%.2f,"rtt_last":%.2f,"rtt_p50":%.2f,"rtt_p95":%.2f,"risk_mode":"%s","regime":"%s","hft_signal":"%s","structure_signal":"%s"})",
                snap.xau_bid,
                snap.xau_ask,
                snap.xag_bid,
                snap.xag_ask,
                snap.hft_pnl,
                snap.strategy_pnl,
                snap.fix_rtt_last,
                snap.fix_rtt_p50,
                snap.fix_rtt_p95,
                snap.hft_regime,
                snap.strategy_regime,
                snap.hft_trigger,
                snap.strategy_trigger
            );
        }else{
            // Fallback to dummy data if shared memory not available
            snprintf(json_buffer, sizeof(json_buffer),
                R"({"xau_bid":0,"xau_ask":0,"xag_bid":0,"xag_ask":0,"hft_pnl":0,"strategy_pnl":0,"rtt_last":0,"rtt_p50":0,"rtt_p95":0,"risk_mode":"WAITING","regime":"DISCONNECTED","hft_signal":"NONE","structure_signal":"NONE"})"
            );
        }
        
        send_text_frame(s, json_buffer);
        Sleep(100); // 10 Hz update rate (faster than original 1 Hz)
    }
}

void handle_client(SOCKET client){
    char buffer[8192];
    int bytes=recv(client,buffer,sizeof(buffer)-1,0);
    if(bytes<=0){closesocket(client);return;}
    buffer[bytes]=0;
    std::string req(buffer);

    if(req.find("Upgrade: websocket")!=std::string::npos){
        std::string marker="Sec-WebSocket-Key:";
        size_t pos=req.find(marker);
        if(pos==std::string::npos){closesocket(client);return;}
        pos+=marker.size();
        size_t end=req.find("\r\n",pos);
        std::string key=req.substr(pos,end-pos);
        while(!key.empty()&&(key[0]==' '||key[0]=='\t')) key.erase(0,1);

        std::string accept=generate_accept_key(key);

        std::ostringstream resp;
        resp<<"HTTP/1.1 101 Switching Protocols\r\n"
            <<"Upgrade: websocket\r\n"
            <<"Connection: Upgrade\r\n"
            <<"Sec-WebSocket-Accept: "<<accept<<"\r\n\r\n";

        std::string r=resp.str();
        send_all(client,r.c_str(),r.size());
        websocket_loop(client);
        closesocket(client);
        return;
    }

    if(req.find("GET /favicon.ico")!=std::string::npos){
        std::string r="HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        send_all(client,r.c_str(),r.size());
        closesocket(client);
        return;
    }

    std::string html=load_dashboard();
    std::ostringstream resp;
    resp<<"HTTP/1.1 200 OK\r\n"
        <<"Content-Type: text/html\r\n"
        <<"Content-Length: "<<html.size()<<"\r\n"
        <<"Connection: close\r\n\r\n"
        <<html;

    std::string r=resp.str();
    send_all(client,r.c_str(),r.size());
    closesocket(client);
}

int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int){
    // Singleton check
    HANDLE hMutex=CreateMutexA(NULL,TRUE,MUTEX_NAME);
    if(!hMutex||GetLastError()==ERROR_ALREADY_EXISTS) return 0;

    // Initialize shared memory connection
    bool sharedMemOk = SnapshotInit();
    
    // Initialize Winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);

    // Setup server socket
    addrinfo hints={},*result=nullptr;
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    hints.ai_flags=AI_PASSIVE;

    getaddrinfo(NULL,PORT,&hints,&result);

    SOCKET server=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    bind(server,result->ai_addr,result->ai_addrlen);
    listen(server,SOMAXCONN);

    // Accept connections
    while(true){
        SOCKET client=accept(server,NULL,NULL);
        std::thread(handle_client,client).detach();
    }

    return 0;
}
