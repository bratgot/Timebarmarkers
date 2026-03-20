import nuke, json

# Prints the current marker data stored on the Root node.
# Use this to verify markers are being saved correctly.

root = nuke.root()

if 'mt_marker_data' in root.knobs():
    raw = root['mt_marker_data'].getValue()
    try:
        markers = json.loads(raw)
        print("Found {} marker(s) on Root node:".format(len(markers)))
        for m in markers:
            print("  Frame {:>6}  |  {:20}  |  {}".format(
                m.get('frame', '?'),
                m.get('label', '(no label)'),
                m.get('color', '?')
            ))
    except Exception as e:
        print("Failed to parse marker data: {}".format(e))
        print("Raw value: {}".format(raw))
else:
    print("No mt_marker_data knob on Root node — no markers saved yet.")
    print("If you had markers from the old version, run delete_old_storage_node.py")
    print("and re-add your markers.")
