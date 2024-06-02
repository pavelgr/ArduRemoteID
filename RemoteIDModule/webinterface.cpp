#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "webinterface.h"
#include "parameters.h"
#include "romfs.h"
#include "check_firmware.h"
#include "status.h"
#include "debug.h"

static WebServer server(80);

/*
  serve files from ROMFS
 */
class ROMFS_Handler : public RequestHandler
{
    bool canHandle(HTTPMethod method, String uri) {
        if (uri == "/") {
            uri = "/index.html";
        }
        uri = "web" + uri;
        if (ROMFS::exists(uri.c_str())) {
            return true;
        }
        return false;
    }

    bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
        if (requestUri == "/") {
            requestUri = "/index.html";
        }
        String uri = "web" + requestUri;
        DPRINTF("handle: '%s'\n", requestUri.c_str());

        // work out content type
        const char *content_type = "text/html";
        const struct {
            const char *extension;
            const char *content_type;
        } extensions[] = {
            { ".js", "text/javascript" },
            { ".jpg", "image/jpeg" },
            { ".png", "image/png" },
            { ".css", "text/css" },
        };
        for (const auto &e : extensions) {
            if (uri.endsWith(e.extension)) {
                content_type = e.content_type;
                break;
            }
        }

        auto *f = ROMFS::find_stream(uri.c_str());
        if (f != nullptr) {
            server.sendHeader("Content-Encoding", "gzip");
            server.streamFile(*f, content_type);
            delete f;
            return true;
        }
        return false;
    }

} ROMFS_Handler;

/*
  serve files from ROMFS
 */
class AJAX_Handler : public RequestHandler
{
    bool canHandle(HTTPMethod method, String uri) {
        return uri == "/ajax/status.json";
    }

    bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
        if (requestUri != "/ajax/status.json") {
            return false;
        }
        server.send(200, "application/json", status_json());
        return true;
    }

} AJAX_Handler;

/*
  init web server
 */
void WebInterface::init(void)
{
    DPRINTF("WAP start %s %s\n", g.wifi_ssid, g.wifi_password);
    IPAddress myIP = WiFi.softAPIP();

    server.addHandler( &AJAX_Handler );
    server.addHandler( &ROMFS_Handler );

    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
        if (Update.hasError()) {
			server.sendHeader("Connection", "close");
		    server.send(500, "text/plain","FAIL");
		    DPRINTF("Update Failed: Update function has errors\n");
		    delay(5000);
		} else {
			server.sendHeader("Connection", "close");
			server.send(200, "text/plain","OK");
			DPRINTF("Update Success: \nRebooting...\n");
			delay(1000);
			ESP.restart();
		}
    }, [this]() {
        HTTPUpload& upload = server.upload();
        static const esp_partition_t* partition_new_firmware = esp_ota_get_next_update_partition(NULL); //get OTA partion to which we will write new firmware file;
        if (upload.status == UPLOAD_FILE_START) {
            DPRINTF("Update: %s\n", upload.filename.c_str());
            lead_len = 0;

            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                DPRINTF("Update Failed: cannot start file write\n");
                //Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* flashing firmware to ESP*/
            if (lead_len < sizeof(lead_bytes)) {
                uint32_t n = sizeof(lead_bytes)-lead_len;
                if (n > upload.currentSize) {
                    n = upload.currentSize;
                }
                memcpy(&lead_bytes[lead_len], upload.buf, n);
                lead_len += n;
            }
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                DPRINTF("Update Failed: failed file write\n");
                //Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            // write extra bytes to force flush of the buffer before we check signature
            uint32_t extra = SPI_FLASH_SEC_SIZE+1;
            while (extra--) {
                uint8_t ff = 0xff;
                Update.write(&ff, 1);
            }
            if (!CheckFirmware::check_OTA_next(partition_new_firmware, lead_bytes, lead_len)) {
                DPRINTF("Update Failed: firmware checks have errors\n");
                server.sendHeader("Connection", "close");
                server.send(500, "text/plain","FAIL");
                delay(5000);
            } else if (Update.end(true)) {
                DPRINTF("Update Success: %u\nRebooting...\n", upload.totalSize);
                server.sendHeader("Connection", "close");
                server.send(200, "text/plain","OK");
            } else {
                //Update.printError(Serial);
                DPRINTF("Update Failed: Update.end function has errors\n");
                server.sendHeader("Connection", "close");
                server.send(500, "text/plain","FAIL");
                delay(5000);
            }
        }
    });
    DPRINTF("WAP started\n");
    server.begin();
}

void WebInterface::update()
{
    if (!initialised) {
        init();
        initialised = true;
    }
    server.handleClient();
}
