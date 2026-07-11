#include "voyage_eta_pi.h"

#include <wx/datetime.h>

#include <cmath>
#include <limits>
#include <memory>

#ifndef DECL_EXP
#ifdef __WXMSW__
#define DECL_EXP __declspec(dllexport)
#else
#define DECL_EXP
#endif
#endif

static const char* voyage_eta_xpm[] = {
"32 32 4 1",
"  c None",
". c #163A5F",
"+ c #4DA3D9",
"@ c #FFFFFF",
"                                ",
"          ............          ",
"       ..................       ",
"     ......................     ",
"    ....++++++++++++++++....    ",
"   ...++++++++++++++++++++...   ",
"  ...++++++@@@@@@@@+++++++...   ",
"  ..+++++++@@@@@@@@++++++++..   ",
" ..++++++++@@++++@@+++++++++..  ",
" ..++++++++@@++++@@+++++++++..  ",
"..+++++++++@@@@@@@@++++++++++.. ",
"..+++++++++@@@@@@@@++++++++++.. ",
"..+++++++++@@++++++++++++++++.. ",
"..+++++++++@@++++++++++++++++.. ",
"..+++++++++@@@@@@@@++++++++++.. ",
"..+++++++++@@@@@@@@++++++++++.. ",
"..+++++++++++++++++++++++++++.. ",
"..+++++@@++++@@++++@@++++++++.. ",
"..+++++@@++++@@++++@@++++++++.. ",
"..+++++@@++++@@++++@@++++++++.. ",
"..+++++@@++++@@++++@@++++++++.. ",
" ..++++@@@@@@@@@@@@@@+++++++..  ",
" ..++++@@@@@@@@@@@@@@+++++++..  ",
"  ..++++++++++++++++++++++++..  ",
"  ...++++++++++++++++++++++...  ",
"   ...++++++++++++++++++++...   ",
"    ....++++++++++++++++....    ",
"     ......................     ",
"       ..................       ",
"          ............          ",
"                                ",
"                                "
};

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
    return new voyage_eta_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* plugin) {
    delete plugin;
}

enum {
    ID_ROUTE = wxID_HIGHEST + 900,
    ID_WAYPOINT_1,
    ID_WAYPOINT_2,
    ID_WAYPOINT_3,
    ID_ACTUAL_SPEED,
    ID_MANUAL_SPEED_MODE,
    ID_MANUAL_SPEED_VALUE,
    ID_REFRESH_TIMER
};

wxBEGIN_EVENT_TABLE(VoyageEtaDialog, wxDialog)
    EVT_CHOICE(ID_ROUTE, VoyageEtaDialog::OnRouteChanged)
    EVT_CHOICE(ID_WAYPOINT_1, VoyageEtaDialog::OnWaypointChanged)
    EVT_CHOICE(ID_WAYPOINT_2, VoyageEtaDialog::OnWaypointChanged)
    EVT_CHOICE(ID_WAYPOINT_3, VoyageEtaDialog::OnWaypointChanged)
    EVT_RADIOBUTTON(ID_ACTUAL_SPEED, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_RADIOBUTTON(ID_MANUAL_SPEED_MODE, VoyageEtaDialog::OnSpeedModeChanged)
    EVT_TEXT(ID_MANUAL_SPEED_VALUE, VoyageEtaDialog::OnManualSpeedChanged)
    EVT_TIMER(ID_REFRESH_TIMER, VoyageEtaDialog::OnTimer)
wxEND_EVENT_TABLE()

voyage_eta_pi::voyage_eta_pi(void* ppimgr)
    : opencpn_plugin_118(ppimgr) {}

voyage_eta_pi::~voyage_eta_pi() = default;

int voyage_eta_pi::Init() {
    if (!m_icon.IsOk()) {
        m_icon = wxBitmap(voyage_eta_xpm);
    }

    m_toolbar_id = InsertPlugInTool(
        "Voyage ETA",
        &m_icon,
        &m_icon,
        wxITEM_NORMAL,
        "Voyage ETA",
        "ETA to three selected waypoints",
        nullptr,
        -1,
        0,
        this
    );

    return WANTS_TOOLBAR_CALLBACK |
           INSTALLS_TOOLBAR_TOOL |
           WANTS_NMEA_EVENTS;
}

bool voyage_eta_pi::DeInit() {
    if (m_dialog) {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }
    return true;
}

wxBitmap* voyage_eta_pi::GetPlugInBitmap() {
    return &m_icon;
}

void voyage_eta_pi::OnToolbarToolCallback(int) {
    if (!m_dialog) {
        m_dialog = new VoyageEtaDialog(GetOCPNCanvasWindow(), this);
    }

    m_dialog->Show();
    m_dialog->Raise();
    m_dialog->ReloadRoutes();
    m_dialog->RefreshValues();
}

void voyage_eta_pi::SetPositionFixEx(PlugIn_Position_Fix_Ex& pfix) {
    if (!std::isfinite(pfix.Lat) || !std::isfinite(pfix.Lon)) {
        return;
    }

    m_lat = pfix.Lat;
    m_lon = pfix.Lon;
    m_sog = std::isfinite(pfix.Sog) ? pfix.Sog : 0.0;
    m_has_fix = true;

    if (m_dialog && m_dialog->IsShown()) {
        m_dialog->RefreshValues();
    }
}

VoyageEtaDialog::VoyageEtaDialog(wxWindow* parent, voyage_eta_pi* plugin)
    : wxDialog(
          parent,
          wxID_ANY,
          "Voyage ETA",
          wxDefaultPosition,
          wxSize(470, 390),
          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_plugin(plugin),
      m_timer(this, ID_REFRESH_TIMER) {

    auto* top = new wxBoxSizer(wxVERTICAL);

    auto* route_row = new wxBoxSizer(wxHORIZONTAL);
    route_row->Add(
        new wxStaticText(this, wxID_ANY, "Route:"),
        0,
        wxALIGN_CENTER_VERTICAL | wxRIGHT,
        8);

    m_route_choice = new wxChoice(this, ID_ROUTE);
    route_row->Add(m_route_choice, 1, wxEXPAND);
    top->Add(route_row, 0, wxEXPAND | wxALL, 12);

    auto* speed_box = new wxStaticBoxSizer(wxVERTICAL, this, "Speed");

    m_use_actual = new wxRadioButton(
        this,
        ID_ACTUAL_SPEED,
        "Use actual SOG",
        wxDefaultPosition,
        wxDefaultSize,
        wxRB_GROUP);

    m_use_manual = new wxRadioButton(
        this,
        ID_MANUAL_SPEED_MODE,
        "Use manual speed");

    auto* manual_row = new wxBoxSizer(wxHORIZONTAL);
    manual_row->Add(m_use_manual, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_manual_speed = new wxTextCtrl(
        this,
        ID_MANUAL_SPEED_VALUE,
        "13.0",
        wxDefaultPosition,
        wxSize(70, -1));

    manual_row->Add(m_manual_speed, 0, wxALIGN_CENTER_VERTICAL);
    manual_row->Add(
        new wxStaticText(this, wxID_ANY, " kn"),
        0,
        wxALIGN_CENTER_VERTICAL);

    auto* actual_row = new wxBoxSizer(wxHORIZONTAL);
    actual_row->Add(
        new wxStaticText(this, wxID_ANY, "Actual SOG:"),
        0,
        wxALIGN_CENTER_VERTICAL | wxRIGHT,
        8);

    m_actual_sog = new wxStaticText(this, wxID_ANY, "--");
    actual_row->Add(m_actual_sog, 0, wxALIGN_CENTER_VERTICAL);

    speed_box->Add(m_use_actual, 0, wxALL, 4);
    speed_box->Add(manual_row, 0, wxALL, 4);
    speed_box->Add(actual_row, 0, wxALL, 4);
    top->Add(speed_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxFont eta_font = GetFont();
    eta_font.SetPointSize(12);
    eta_font.SetWeight(wxFONTWEIGHT_BOLD);

    auto add_waypoint = [&](const wxString& title,
                            int id,
                            wxChoice*& choice,
                            wxStaticText*& eta) {
        auto* box = new wxStaticBoxSizer(wxVERTICAL, this, title);

        choice = new wxChoice(this, id);
        box->Add(choice, 0, wxEXPAND | wxALL, 6);

        auto* eta_row = new wxBoxSizer(wxHORIZONTAL);
        eta_row->Add(
            new wxStaticText(this, wxID_ANY, "ETA:"),
            0,
            wxALIGN_CENTER_VERTICAL | wxRIGHT,
            8);

        eta = new wxStaticText(this, wxID_ANY, "--");
        eta->SetFont(eta_font);
        eta_row->Add(eta, 1, wxALIGN_CENTER_VERTICAL);

        box->Add(eta_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
        top->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    };

    add_waypoint("Waypoint 1", ID_WAYPOINT_1, m_waypoint_1, m_eta_1);
    add_waypoint("Waypoint 2", ID_WAYPOINT_2, m_waypoint_2, m_eta_2);
    add_waypoint("Waypoint 3", ID_WAYPOINT_3, m_waypoint_3, m_eta_3);

    m_status = new wxStaticText(
        this,
        wxID_ANY,
        "Waiting for route and position.");

    top->Add(m_status, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizerAndFit(top);
    CentreOnParent();

    m_use_actual->SetValue(true);
    m_manual_speed->Enable(false);

    ReloadRoutes();
    m_timer.Start(1000);
}

void VoyageEtaDialog::ReloadRoutes() {
    m_route_choice->Clear();
    m_waypoint_1->Clear();
    m_waypoint_2->Clear();
    m_waypoint_3->Clear();
    m_route_guids.Clear();
    m_points.clear();

    const wxArrayString guids = GetRouteGUIDArray();

    if (guids.IsEmpty()) {
        m_status->SetLabel("No OpenCPN routes found.");
        return;
    }

    for (size_t i = 0; i < guids.GetCount(); ++i) {
        const wxString guid = guids.Item(i);
        std::unique_ptr<PlugIn_Route> route(GetRoute_Plugin(guid));

        if (!route) {
            continue;
        }

        wxString name = route->m_NameString;
        if (name.IsEmpty()) {
            name = guid;
        }

        m_route_choice->Append(name);
        m_route_guids.Add(guid);
    }

    if (m_route_choice->GetCount() > 0) {
        m_route_choice->SetSelection(0);
        LoadSelectedRoute();
    }
}

bool VoyageEtaDialog::LoadSelectedRoute() {
    m_waypoint_1->Clear();
    m_waypoint_2->Clear();
    m_waypoint_3->Clear();
    m_points.clear();

    const int route_selection = m_route_choice->GetSelection();

    if (route_selection == wxNOT_FOUND ||
        route_selection >= static_cast<int>(m_route_guids.GetCount())) {
        return false;
    }

    std::unique_ptr<PlugIn_Route> route(
        GetRoute_Plugin(m_route_guids.Item(route_selection)));

    if (!route || !route->pWaypointList) {
        return false;
    }

    size_t index = 1;

    for (Plugin_WaypointList::iterator it = route->pWaypointList->begin();
         it != route->pWaypointList->end();
         ++it) {

        PlugIn_Waypoint* waypoint = *it;
        if (!waypoint) {
            continue;
        }

        RoutePoint point;
        point.lat = waypoint->m_lat;
        point.lon = waypoint->m_lon;
        point.name = waypoint->m_MarkName;

        if (point.name.IsEmpty()) {
            point.name = wxString::Format("WP %zu", index);
        }

        m_points.push_back(point);

        const wxString label =
            wxString::Format("%zu - %s", index, point.name);

        m_waypoint_1->Append(label);
        m_waypoint_2->Append(label);
        m_waypoint_3->Append(label);

        ++index;
    }

    if (m_points.empty()) {
        m_status->SetLabel("The selected route has no waypoints.");
        return false;
    }

    const int count = static_cast<int>(m_points.size());
    m_waypoint_1->SetSelection(0);
    m_waypoint_2->SetSelection(count > 1 ? count / 2 : 0);
    m_waypoint_3->SetSelection(count - 1);

    RefreshValues();
    return true;
}

double VoyageEtaDialog::SelectedSpeed() const {
    if (m_use_actual->GetValue()) {
        return m_plugin->Sog();
    }

    double speed = 0.0;
    if (!m_manual_speed->GetValue().ToDouble(&speed)) {
        return 0.0;
    }

    return speed;
}

double VoyageEtaDialog::DistanceNm(
    double lat1,
    double lon1,
    double lat2,
    double lon2) {

    constexpr double pi = 3.14159265358979323846;
    constexpr double earth_nm = 3440.065;
    const double rad = pi / 180.0;

    const double phi1 = lat1 * rad;
    const double phi2 = lat2 * rad;
    const double delta_phi = (lat2 - lat1) * rad;
    const double delta_lambda = (lon2 - lon1) * rad;

    const double a =
        std::sin(delta_phi / 2.0) * std::sin(delta_phi / 2.0) +
        std::cos(phi1) * std::cos(phi2) *
        std::sin(delta_lambda / 2.0) * std::sin(delta_lambda / 2.0);

    const double c =
        2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return earth_nm * c;
}

double VoyageEtaDialog::RemainingDistanceNm(size_t destination_index) const {
    if (!m_plugin->HasFix() ||
        m_points.empty() ||
        destination_index >= m_points.size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    size_t nearest_index = 0;
    double nearest_distance = std::numeric_limits<double>::max();

    for (size_t i = 0; i <= destination_index; ++i) {
        const double distance = DistanceNm(
            m_plugin->Latitude(),
            m_plugin->Longitude(),
            m_points[i].lat,
            m_points[i].lon);

        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest_index = i;
        }
    }

    double total_distance = nearest_distance;

    for (size_t i = nearest_index; i < destination_index; ++i) {
        total_distance += DistanceNm(
            m_points[i].lat,
            m_points[i].lon,
            m_points[i + 1].lat,
            m_points[i + 1].lon);
    }

    return total_distance;
}

wxString VoyageEtaDialog::FormatEtaLocal(double hours) {
    if (!std::isfinite(hours) || hours < 0.0) {
        return "--";
    }

    const long minutes =
        static_cast<long>(std::llround(hours * 60.0));

    const wxDateTime eta =
        wxDateTime::Now() + wxTimeSpan::Minutes(minutes);

    return eta.Format("%d %b %Y %H:%M LT");
}

void VoyageEtaDialog::UpdateEta(
    wxChoice* choice,
    wxStaticText* eta_label) {

    const int selection = choice->GetSelection();
    const double speed = SelectedSpeed();

    if (!m_plugin->HasFix() ||
        selection == wxNOT_FOUND ||
        !std::isfinite(speed) ||
        speed < 0.2) {
        eta_label->SetLabel("--");
        return;
    }

    const double distance =
        RemainingDistanceNm(static_cast<size_t>(selection));

    if (!std::isfinite(distance)) {
        eta_label->SetLabel("--");
        return;
    }

    eta_label->SetLabel(FormatEtaLocal(distance / speed));
}

void VoyageEtaDialog::RefreshValues() {
    m_actual_sog->SetLabel(
        wxString::Format("%.1f kn", m_plugin->Sog()));

    UpdateEta(m_waypoint_1, m_eta_1);
    UpdateEta(m_waypoint_2, m_eta_2);
    UpdateEta(m_waypoint_3, m_eta_3);

    if (!m_plugin->HasFix()) {
        m_status->SetLabel("No own-ship position received from OpenCPN.");
        return;
    }

    if (m_points.empty()) {
        m_status->SetLabel("No route waypoints available.");
        return;
    }

    const double speed = SelectedSpeed();

    if (!std::isfinite(speed) || speed < 0.2) {
        m_status->SetLabel("Speed must be at least 0.2 kn.");
        return;
    }

    m_status->SetLabel(
        m_use_actual->GetValue()
            ? "Using live actual SOG."
            : "Using manual speed.");
}

void VoyageEtaDialog::OnRouteChanged(wxCommandEvent&) {
    LoadSelectedRoute();
}

void VoyageEtaDialog::OnWaypointChanged(wxCommandEvent&) {
    RefreshValues();
}

void VoyageEtaDialog::OnSpeedModeChanged(wxCommandEvent&) {
    m_manual_speed->Enable(m_use_manual->GetValue());
    RefreshValues();
}

void VoyageEtaDialog::OnManualSpeedChanged(wxCommandEvent&) {
    if (m_use_manual->GetValue()) {
        RefreshValues();
    }
}

void VoyageEtaDialog::OnTimer(wxTimerEvent&) {
    RefreshValues();
}
