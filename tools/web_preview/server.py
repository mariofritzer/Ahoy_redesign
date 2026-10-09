#!/usr/bin/env python3
"""Local preview of the AhoyDTU web UI with mocked API data.

usage: python3 server.py [port] [lang]   (lang: de | en)
Open http://localhost:8080/live?dark=0
"""
import json, os, re, sys, time
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

HERE = os.path.dirname(os.path.abspath(__file__))
HTML = os.path.join(HERE, "..", "..", "src", "web", "html")
LANG_FILE = os.path.join(HERE, "..", "..", "src", "web", "lang.json")
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
LANG = sys.argv[2] if len(sys.argv) > 2 else "de"
DARK = {"v": False}

PAGES = {"/": "index.html", "/live": "visualization.html", "/setup": "setup.html", "/update": "update.html",
         "/system": "system.html", "/history": "history.html", "/serial": "serial.html", "/about": "about.html",
         "/login": "login.html"}


def translate(fname, data):
    lang = json.load(open(LANG_FILE))
    for f in lang["files"]:
        if f["name"] in (fname, "general"):
            for e in f["list"]:
                data = data.replace("{#" + e["token"] + "}", e[LANG])
    return data


def render(fname):
    p = open(os.path.join(HTML, fname)).read()
    inc = lambda n: open(os.path.join(HTML, "includes", n)).read()
    p = p.replace("{#HTML_HEADER}", inc("header.html")).replace("{#HTML_NAV}", inc("nav.html")).replace("{#HTML_FOOTER}", inc("footer.html"))
    p = re.sub(r"<!--(IF|ENDIF)_[A-Z_]+-->", "", p)
    p = p.replace("{#VERSION}", "0.8.156").replace("{#VERSION_FULL}", "0.8.156").replace("{#VERSION_GIT}", "GIT SHA: preview :: 0.8.156")
    p = translate(fname, p)
    if "open" in PREVIEW_FLAGS:
        p = p.replace("</body>", "<script>setTimeout(()=>{var b=document.getElementsByClassName('s_collapsible');for(var i of [0,7])if(b[i])b[i].click();},600)</script></body>")
    return p


PREVIEW_FLAGS = set()

GENERIC = {"wifi_rssi": -58, "ts_uptime": 93412, "ts_now": int(time.time()), "version": "0.8.156", "modules": "MDH",
           "build": "preview", "env": "esp32-wroom32-de", "host": "AhoyDTU", "menu_prot": False, "menu_mask": 61,
           "menu_protEn": False, "cst_lnk": "", "cst_lnk_txt": "", "region": 0, "timezone": 1, "esp_type": "ESP32"}

IVS = [
    {"name": "HMS-1600 Dach", "status": 2, "power_limit_read": 100, "max_pwr": 1600, "alarm_cnt": 3,
     "pac": 1062.4, "yd": 4821, "yt": 2311.7, "temp": 34.1,
     "ch": [(38.1, 7.12, 271.3, 1421, 512.3, 67.8, 288.6), (37.9, 7.01, 265.7, 1388, 498.1, 66.4, 281.2),
            (36.4, 6.12, 222.8, 1051, 412.9, 55.7, 240.1), (37.0, 7.35, 272.0, 961, 401.2, 68.0, 290.2)],
     "names": ["Ost 1", "Ost 2", "West 1", "West 2"]},
    {"name": "HM-600 Garage", "status": 0, "power_limit_read": 100, "max_pwr": 600, "alarm_cnt": 0,
     "pac": 0, "yd": 1312, "yt": 531.9, "temp": 21.4,
     "ch": [(0, 0, 0, 671, 270.1, 0, 241.0), (0, 0, 0, 641, 261.8, 0, 236.4)],
     "names": ["Garage S", "Garage W"]},
]


def inverter(i):
    iv = IVS[i]
    ch0 = [231.2, 4.61, iv["pac"], 50.01, 0.999, iv["temp"], iv["yt"], iv["yd"], iv["pac"] * 1.04 if iv["pac"] else 0,
           96.2 if iv["pac"] else 0, 1.2, 1281.0 if i == 0 else 488.0, 47.3]
    chs = [ch0] + [list(c) for c in iv["ch"]]
    return {"id": i, "name": iv["name"], "status": iv["status"], "power_limit_read": iv["power_limit_read"],
            "max_pwr": iv["max_pwr"], "alarm_cnt": iv["alarm_cnt"], "ts_max_ac_pwr": int(time.time()) - 7200,
            "ts_max_temp": int(time.time()) - 5400, "ch": chs, "ch_name": ["AC"] + iv["names"],
            "ch_max_pwr": [None] + [440] * len(iv["ch"]), "ts_last_success": int(time.time()) - 12 if iv["status"] else int(time.time()) - 21000,
            "rssi": -71, "generation": 2}


def live():
    return {"generic": GENERIC, "refresh": 5, "max_total_pwr": 1612.3, "iv": [True, True],
            "fld_units": ["V", "A", "W", "Wh", "kWh", "%"]}


def history(day):
    import math
    vals = []
    n = 96 if day else 128
    for k in range(n):
        t = k / (n - 1)
        v = max(0, math.sin(t * math.pi)) ** 1.5 * 1500
        if 0.38 < t < 0.45: v *= 0.6
        vals.append(int(v))
    now = int(time.time())
    return {"generic": GENERIC, "value": vals, "lastValueTs": now, "refresh": 300 if day else 30, "max": max(vals),
            "yld": 6.13, "ts_start": now - n * 300}


class H(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def send(self, body, ctype="text/html; charset=utf-8", code=200):
        b = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_POST(self):
        self.send(json.dumps({"success": True}), "application/json")

    def do_GET(self):
        u = urlparse(self.path)
        q = parse_qs(u.query)
        if "dark" in q:
            DARK["v"] = q["dark"][0] == "1"
        PREVIEW_FLAGS.clear()
        if "open" in q:
            PREVIEW_FLAGS.add("open")
        p = u.path
        if p in PAGES:
            return self.send(render(PAGES[p]))
        if p == "/colors.css":
            f = "colorDark.css" if DARK["v"] else "colorBright.css"
            return self.send(open(os.path.join(HTML, f)).read(), "text/css")
        if p in ("/style.css", "/api.js"):
            return self.send(open(os.path.join(HTML, p[1:])).read(), "text/css" if p.endswith("css") else "application/javascript")
        if p == "/api/live":
            return self.send(json.dumps(live()), "application/json")
        if p.startswith("/api/inverter/id/"):
            return self.send(json.dumps(inverter(int(p.split("/")[-1]))), "application/json")
        if p in ("/api/generic",):
            return self.send(json.dumps(GENERIC), "application/json")
        if p == "/api/powerHistory":
            return self.send(json.dumps(history(False)), "application/json")
        if p in ("/api/powerHistoryDay", "/api/yieldDayHistory"):
            return self.send(json.dumps(history(True)), "application/json")
        if p == "/api/index":
            return self.send(json.dumps({"generic": GENERIC, "ts_now": int(time.time()), "ts_sunrise": int(time.time()) - 30000,
                                         "ts_sunset": int(time.time()) + 10000, "ts_offset": 0, "disNightComm": True,
                                         "inverter": [{"enabled": True, "id": 0, "name": IVS[0]["name"], "cur_pwr": 1062, "is_avail": True, "is_producing": True, "ts_last_success": int(time.time()) - 12},
                                                      {"enabled": True, "id": 1, "name": IVS[1]["name"], "cur_pwr": 0, "is_avail": False, "is_producing": False, "ts_last_success": int(time.time()) - 21000}],
                                         "warnings": [], "infos": []}), "application/json")
        self.send("{}", "application/json", 404)


HTTPServer(("127.0.0.1", PORT), H).serve_forever()
