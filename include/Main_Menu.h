#pragma once

struct Context;

struct Main_Menu {
public:
    enum transferType {
        Sent,
        Incoming
    };

    
    Main_Menu(Context* context);
    ~Main_Menu();

    void draw();
    void set_state(bool state);
    bool get_state() const;
    void addClients(std::array<const char*,3> &clientList);
    bool openFile();
    void addEvent(transferType type,  const char *desc,  const char *fname);
    void incomingRequest(const char *desc, const char *fname);

    
    ImVec2 io;


private:
    bool open;
    Context* ctx;
    std::string sSelectedFile;
    std::string sFilePath;
};
