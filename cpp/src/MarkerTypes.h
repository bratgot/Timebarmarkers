#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

// ─── Marker POD ──────────────────────────────────────────────────────────────
struct Marker
{
    int         frame  = 1;
    std::string label;
    std::string color  = "#E05252";   // hex RGB

    bool operator<(const Marker& o) const { return frame < o.frame; }
};

// ─── Minimal JSON helpers (no third-party deps) ───────────────────────────────
// Format: [{"frame":1,"label":"foo","color":"#E05252"}, ...]

namespace MarkerJson
{

// Escape a string for JSON output
inline std::string escape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else                out += c;
    }
    return out;
}

// Read a string value for a given key from a flat JSON object string
// e.g. key="label"  src={"frame":1,"label":"hello","color":"#fff"}
inline std::string readString(const std::string& src, const std::string& key)
{
    std::string needle = "\"" + key + "\":\"";
    auto pos = src.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    auto end = src.find('"', pos);
    if (end == std::string::npos) return {};
    return src.substr(pos, end - pos);
}

inline int readInt(const std::string& src, const std::string& key)
{
    std::string needle = "\"" + key + "\":";
    auto pos = src.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    while (pos < src.size() && (src[pos] == ' ')) ++pos;
    int val = 0;
    bool neg = false;
    if (pos < src.size() && src[pos] == '-') { neg = true; ++pos; }
    while (pos < src.size() && std::isdigit(src[pos]))
        val = val * 10 + (src[pos++] - '0');
    return neg ? -val : val;
}

// Deserialise the full JSON array
inline std::vector<Marker> fromJson(const std::string& json)
{
    std::vector<Marker> out;
    if (json.empty() || json == "[]") return out;

    // Split on "},{" to get individual object strings
    size_t pos = 0;
    while (pos < json.size()) {
        auto start = json.find('{', pos);
        if (start == std::string::npos) break;
        auto end = json.find('}', start);
        if (end == std::string::npos) break;
        std::string obj = json.substr(start, end - start + 1);
        Marker m;
        m.frame = readInt(obj, "frame");
        m.label = readString(obj, "label");
        m.color = readString(obj, "color");
        if (m.color.empty()) m.color = "#E05252";
        out.push_back(m);
        pos = end + 1;
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Serialise to JSON array
inline std::string toJson(const std::vector<Marker>& markers)
{
    if (markers.empty()) return "[]";
    std::ostringstream ss;
    ss << '[';
    for (size_t i = 0; i < markers.size(); ++i) {
        const auto& m = markers[i];
        if (i) ss << ',';
        ss << "{\"frame\":" << m.frame
           << ",\"label\":\"" << escape(m.label) << '"'
           << ",\"color\":\"" << escape(m.color)  << "\"}";
    }
    ss << ']';
    return ss.str();
}

} // namespace MarkerJson
