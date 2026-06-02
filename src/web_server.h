#pragma once

// ---------------------------------------------------------------------------
// web_server.h – ESPAsyncWebServer wrapper.
// ---------------------------------------------------------------------------

// Registers all routes and starts the HTTP server on port 80.
// Call after WiFi is up.
void web_server_begin();
