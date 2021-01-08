/*
    The idea of this program was to 'hack' CounterStrike:GlobalOffensive
    The program consists of a radar-hack. A radar-hack makes the enemy players
        visible on the radar
    First of all, we want to get all of the running processes and search
        for 'csgo.exe' and open a handle with the access all attribute
        to be able to read and write memory from the game, if we find it search
        for module 'client.dll' to get its base address used
        later to find the addresses that we want to read and write
    For the enemy players to show on our radar, we need to set their spotted
        status from 0 to 1, iterating through all entities and checking if they are
        players, not NULL entities
*/
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

using namespace std;

///Forward declarations:
/// Function used to display errors if needed
void error(const char* error_title, const char* error_message);
/// Function for finding the process we are looking for and opening a handle to it
void GetProcess(char* procName);
/// Function for finding the module we are looking for from the obtained process
uintptr_t Module(LPSTR ModuleName);

/// Global variables
HANDLE hProc;
PROCESSENTRY32 ProcEntry;
DWORD pID;

/// Creating a structure to keep the offsets that we want to use. The offsets are constant (might change with an update)
struct Offseet{
    DWORD dwEntityList = 0x4DA2E14;
    DWORD m_bSpotted = 0x93D;
}offset;

/// Creating a template for ReadProcessMemory and WriteProcessMemory to read and write the data type we need
template <class MyType>
/// Giving to the function a value and an address. The function will write the value at the specified address
void WPM(DWORD Address, MyType value){
    WriteProcessMemory(hProc, (LPVOID)Address, &value, sizeof(MyType), NULL);
}

template <class MyType>
/// Read from the address specified
MyType RPM(DWORD Address){
    MyType value;
    ReadProcessMemory(hProc, (LPVOID)Address, &value, sizeof(MyType), NULL);
    return value;
}


int main(){
    /// Get the process and open the handle
    GetProcess("csgo.exe");
    /// Get the base address of the module, the function returns a uintptr_t and we want it to be DWORD
    DWORD engineDLLBase = (DWORD)Module("client.dll");
    /// While we do not close the program we want it to run
    while(true){

        /// Iterate through all of the game entities (64 entities)
        for(short int i = 0; i < 64; i ++){
            /// Get the entity address based on entity index
            DWORD entity = RPM<DWORD>(engineDLLBase + offset.dwEntityList + i*0x10);
            /// If the entity found is not null (it s a player)
            if(entity != NULL){
                /// The data we want to write it s a boolean type, the player is either visible or not
                /// We want to make every player visible (our teammates are already visible so are we,
                                                    ///     these values will not change if we set them to 1
                WPM<bool>(entity + offset.m_bSpotted, 1);
            }
        }
        /// 'Cool down' the program
        Sleep(1);
    }
}


void error(const char* error_title, const char* error_message){
    /// Pop up a message box with the title and message needed
    MessageBox(NULL, error_message, error_title, NULL);
    exit(-1);
}

/// Get the process wanted and open a handle with all access attribute
void GetProcess(char* procName){
    /// Set the size of ProcEntry before using it
    ProcEntry.dwSize = sizeof(ProcEntry);
    /// Create a snapshot of all the processes
    hProc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    /// Check if the snapshot has been created, if not display and error
    if(hProc == INVALID_HANDLE_VALUE){error("ProcessSnapshot", "The process snapshot could not be created");}
    /// Iterate through all the processes with Process32Next
    while(Process32Next(hProc, &ProcEntry)){
        /// If we find the process we are looking for
        if(!strcmp((char*)ProcEntry.szExeFile, procName)){
            /// Get the process ID
            pID = ProcEntry.th32ProcessID;
            /// Close the snapshot handle
            CloseHandle(hProc);
            /// Create a handle with all access to the process ID
            hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
            /// Exit the function
            return;
        }
    }
    /// If the module specified is not found display an error
    error("ProcessNotFound", "The process specified could not be found");
}

/// Get the base address of the module wanted
uintptr_t Module(LPSTR ModuleName){
    /// Create the snapshot of the process ID modules
    HANDLE hMod = CreateToolhelp32Snapshot(TH32CS_SNAPALL | TH32CS_SNAPMODULE32, pID);
    /// Check if the module snapshot has been created, if not display an error
    if(hMod == INVALID_HANDLE_VALUE){error("ModuleSnapshot", "The module snapshot could not be created");}
    /// Create the variable for keeping the module entry
    MODULEENTRY32 mEntry;
    /// Set the size of the of the mEntry before using it
    mEntry.dwSize = sizeof(mEntry);
    /// Iterate through all the modules with Module32Next
    while(Module32Next(hMod, &mEntry)){
        /// If we find the module we are looking for
        if(!strcmp((char*)mEntry.szModule, ModuleName)){
            /// Close the module snapshot
            CloseHandle(hMod);
            /// Return the module base address as uintptr_t
            return (uintptr_t)mEntry.modBaseAddr;
        }
    }
    /// If the module specified is not found display an error
    error("ModuleNotFound", "The module specified could not be found in the process ID");
}
