set(OCPN_TEST_REPO
    "s4angeloquebral-ai/voyage-eta-alpha"
    CACHE STRING "Default repository for untagged builds"
)

set(OCPN_BETA_REPO
    "s4angeloquebral-ai/voyage-eta-beta"
    CACHE STRING "Default repository for beta builds"
)

set(OCPN_RELEASE_REPO
    "s4angeloquebral-ai/voyage-eta-prod"
    CACHE STRING "Default repository for release builds"
)

set(PKG_NAME voyage_eta_pi)
set(PKG_VERSION 1.0.0)
set(PKG_PRERELEASE "")
set(DISPLAY_NAME "Voyage ETA")
set(PLUGIN_API_NAME "Voyage ETA")
set(PKG_SUMMARY "ETA to three selected route waypoints")
set(PKG_DESCRIPTION [=[
Shows ETA to three selected waypoints using live SOG or manual speed.
]=])
set(PKG_AUTHOR "Captain Domingo Angelo Quebral Jr")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://github.com/s4angeloquebral-ai/VoyageETA_pi)
set(PKG_INFO_URL https://github.com/s4angeloquebral-ai/VoyageETA_pi)

set(SRC
    src/voyage_eta_pi.h
    src/voyage_eta_pi.cpp
)

set(PKG_API_LIB api-18)

macro(late_init)
endmacro()

macro(add_plugin_libraries)
endmacro()
