// Exercise the actual ImGui presentation on CPU. No window, game, GPU or network.
#include "../src/OverlayA19.cpp"
#include "imgui_internal.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cmath>
namespace qp { void Log(const char*, ...) {} }

int main() {
    namespace fs=std::filesystem;
    fs::path dir=fs::temp_directory_path()/(L"QProtocol-MenuSmoke-"+std::to_wstring(GetCurrentProcessId()));
    fs::create_directories(dir);
    const fs::path ini=dir/L"QProtocol.ini";
    if(!qp::updates::EnsureDefaultIni(ini.wstring())) return 1;
    { std::ofstream out(ini,std::ios::app|std::ios::binary); out << "\n; personal settings retained\n"; }
    auto read=[&](){std::ifstream f(ini,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});};
    const auto personal=read();
    if(!qp::updates::EnsureDefaultIni(ini.wstring()) || read()!=personal) return 2;
    for(const ImVec2 display : {ImVec2(640,480),ImVec2(900,720),ImVec2(1280,900)}) {
        ImGui::CreateContext();
        auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
        io.DisplaySize=display;io.DeltaTime=1.0f/60;
        unsigned char* pixels=nullptr;int w=0,h=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
        qp::g_iniPath=ini.wstring();qp::g_uiLoaded=false;qp::ApplyStyle();
        for(int frame=0;frame<3;++frame) {
            ImGui::NewFrame();qp::DrawOverlayWindow();ImGui::Render();
            auto* window=ImGui::FindWindowByName("Q Protocol");
            if(!window || window->Size.x>display.x || window->Size.y>display.y) return 3;
            auto* data=ImGui::GetDrawData();
            if(!data || !data->Valid || (frame==2 && data->TotalVtxCount==0)) return 4;
            for(int n=0;n<data->CmdListsCount;++n) for(const auto& v:data->CmdLists[n]->VtxBuffer)
                if(!std::isfinite(v.pos.x)||!std::isfinite(v.pos.y)) return 5;
        }
        // Also construct the real Updates tab without starting its worker.
        ImGui::NewFrame();ImGui::Begin("Updates test");qp::DrawUpdatesTab();ImGui::End();ImGui::Render();
        ImGui::DestroyContext();
    }
    if(read()!=personal) return 6;
    fs::remove_all(dir);
    printf("Menu/bootstrap smoke PASS: 3 viewport sizes, real Updates tab, default INI creation and byte-preserving reuse. No game or network.\n");
    return 0;
}
