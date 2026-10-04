"""Source contracts complement live sandbox PHP/HTTP tests and visual checks."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
php = (root / "DA-Web/api/server_info.php").read_text()
js = (root / "DA-Web/js/app.js").read_text()
http = (root / "tools/serverd/http_server.cpp").read_text()
spa = (root / "tools/serverd/spa_index.h").read_text()
assert "typedef unsigned int ADSHANDLE;" in php
assert "OAdsGetServerStats(unsigned int," in php
assert "hash_equals($_SESSION['management_csrf']" in php
assert "application/json" in php and "Use HTTPS to enter management credentials" in php
assert "'expires' => time() + 1200" in php
assert "($mgAuth['endpoint'] ?? '') !== $endpointKey" in php
assert "($connInfo['username']" not in php and "($connInfo['password']" not in php
assert "form.elements.password.value = '';" in js
assert "err.data?.management_required" in js
assert "serverHealthHtml(resp.health,resp.health_error)" in js
assert "value == null" not in spa or "Unavailable" in spa
assert 'srv.Get("/api/server/health"' in http
assert "openads::mgmt::health_json(wire_srv_->build_mg_snapshot()" in http
assert "Cache-Control" in http
assert 'renderServerHealth(await api("/api/server/health"))' in spa
assert "unset($_SESSION['management_auth'][$name]);" in (root / "DA-Web/api/connect.php").read_text()
print("portal health source contracts passed")

server_info = js[js.index("  function loadServerInfo("):]
assert "if (resp.management_required)" in server_info
assert "id=\"mg-logout-${tabId}\"" in server_info
assert "management_required" not in js[:js.index("  function serverHealthHtml(")]
