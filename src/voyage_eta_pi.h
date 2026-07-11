#pragma once

#include "ocpn_plugin.h"

#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/wx.h>

#include <vector>

class VoyageEtaDialog;

class voyage_eta_pi final : public opencpn_plugin_118 {
public:
    explicit voyage_eta_pi(void* ppimgr);
    ~voyage_eta_pi() override;

    int Init() override;
    bool DeInit() override;

    int GetAPIVersionMajor() override { return 1; }
    int GetAPIVersionMinor() override { return 18; }

    int GetPlugInVersionMajor() override { return 1; }
    int GetPlugInVersionMinor() override { return 0; }
    int GetPlugInVersionPatch() override { return 0; }
    int GetPlugInVersionPost() override { return 0; }

    wxString GetCommonName() override { return "Voyage ETA"; }
    wxString GetShortDescription() override {
        return "ETA to three selected route waypoints";
    }
    wxString GetLongDescription() override {
        return "Shows ETA to three selected OpenCPN route waypoints using live SOG or manual speed.";
    }

    wxBitmap* GetPlugInBitmap() override;
    int GetToolbarToolCount() override { return 1; }

    void OnToolbarToolCallback(int id) override;
    void SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) override;

    bool HasFix() const { return m_has_fix; }
    double Latitude() const { return m_lat; }
    double Longitude() const { return m_lon; }
    double Sog() const { return m_sog; }

private:
    VoyageEtaDialog* m_dialog = nullptr;
    int m_toolbar_id = -1;
    wxBitmap m_icon;

    bool m_has_fix = false;
    double m_lat = 0.0;
    double m_lon = 0.0;
    double m_sog = 0.0;
};

struct RoutePoint {
    wxString name;
    double lat = 0.0;
    double lon = 0.0;
};

class VoyageEtaDialog final : public wxDialog {
public:
    VoyageEtaDialog(wxWindow* parent, voyage_eta_pi* plugin);
    ~VoyageEtaDialog() override = default;

    void ReloadRoutes();
    void RefreshValues();

private:
    void OnRouteChanged(wxCommandEvent&);
    void OnWaypointChanged(wxCommandEvent&);
    void OnSpeedModeChanged(wxCommandEvent&);
    void OnManualSpeedChanged(wxCommandEvent&);
    void OnTimer(wxTimerEvent&);

    bool LoadSelectedRoute();
    double SelectedSpeed() const;
    double RemainingDistanceNm(size_t destination_index) const;

    static double DistanceNm(double lat1, double lon1, double lat2, double lon2);
    static wxString FormatEtaLocal(double hours);

    void UpdateEta(wxChoice* choice, wxStaticText* eta_label);

    voyage_eta_pi* m_plugin = nullptr;

    wxChoice* m_route_choice = nullptr;

    wxRadioButton* m_use_actual = nullptr;
    wxRadioButton* m_use_manual = nullptr;
    wxTextCtrl* m_manual_speed = nullptr;
    wxStaticText* m_actual_sog = nullptr;

    wxChoice* m_waypoint_1 = nullptr;
    wxChoice* m_waypoint_2 = nullptr;
    wxChoice* m_waypoint_3 = nullptr;

    wxStaticText* m_eta_1 = nullptr;
    wxStaticText* m_eta_2 = nullptr;
    wxStaticText* m_eta_3 = nullptr;

    wxStaticText* m_status = nullptr;

    wxArrayString m_route_guids;
    std::vector<RoutePoint> m_points;
    wxTimer m_timer;

    wxDECLARE_EVENT_TABLE();
};
