from pyzabbix import ZabbixAPI
import sys
server = "http://127.0.0.1/zabbix"
user = "Admin"
pw = "zabbix"

try:
    group_name=sys.argv[1]
    template_name=sys.argv[2]
except Exception:
    print "usage: %s group_name template_name"%sys.argv[0]
    sys.exit(1)

interfaces=[
            {
                "type": 1,
                "main": 1,
                "useip": 1,
                "ip": "127.0.0.1",
                "dns": "",
                "port": "10050"
            }
        ]

zapi = ZabbixAPI(server)

# Login to the Zabbix API
zapi.login(user, pw)

def get_items(hostid, key, loop=None, asic=None, miner=None):
    #params=["total","avg","sum"]
    keys = []
    if miner:
        keys.append("miner.%s" % (key))
    #   keys.extend(["miner.%s[%s]" % (key,p) for p in params])
    if loop:
        if type(loop) != type([]): loop = [loop]
        keys.extend(["loop%d.%s" % (l,key) for l in loop])
    #   keys.extend(["loop%d.%s[%s]" % (l,key,p) for l in loop for p in params])
    if asic:
        if type(asic) != type([]): asic = [asic]
        keys.extend(["loop%d.asic%d.%s" % (l,a,key) for l in loop for a in asic])
    print keys
    return zapi.item.get(hostids=hostid,filter={"key_": keys}, output="itemids")

def create_graph(name, items, settings, graph_settings):
    if type(items) != type([]): items = [];
    if type(settings) != type([]): settings = [settings]
    # graph!
    graphlines = []
    base = {
        "drawtype":"0",  # 0 = line, 1=filled, 2=bold, 3=dot, 4=dashed, 5=gradient
        "yaxisside":"0", # 0=left,1=right
        "calc_fnc":"2", # 1 = min, 2 = avg, 4 = max, 7 = all
        "type":"0",  # ??
        "periods_cnt":"5"
    }
    for item_list,setting in zip(items, settings): # NOTE i is a list, not a single item!!!!
        t=base.copy()
        t.update(setting) 
        for c, iid in zip(range(len(item_list)),item_list):
            t.update({
                "sortorder": 1+c,
                "itemid": int(iid["itemid"]),
                "color": "".join(["%s"%"3456789abc"[((7+c)+7*(i+c))%10] for i in range(6)])
                })
            graphlines.append(t.copy())
    graph = {
        "gitems":graphlines,
        "name":name,
        "width":"900",
        "height":"200",
        #"yaxismin":"0.0000",
        #"yaxismax":"3.0000",
        #"templateid":"0", # this is for a different graph which serves as a template for this one, not the host template it is associated with
        "show_work_period":"0",
        "show_triggers":"1",
        "graphtype":"0",
        "show_legend":"1",
        "show_3d":"0",
        "percent_left":"0.0000",
        "percent_right":"0.0000",
        "ymin_type":"0",
        "ymax_type":"0",
        "ymin_itemid":"0",
        "ymax_itemid":"0"
        }
    graph.update(graph_settings)
    return int(zapi.graph.create(graph)["graphids"][0])


def main():


    #test if group exists
    if not zapi.hostgroup.exists(name=group_name):
        host_group_id = zapi.hostgroup.create(name=group_name)["groupids"][0]
    else:
        host_group_id = zapi.hostgroup.get(name=group_name)[0]["groupid"]

    print "using group id #", host_group_id

    # check for base template
    if not zapi.template.exists(name=template_name):
        zapi.template.create(host=template_name,interfaces=interfaces,groups=[{"groupid": host_group_id}])
    template_id = int(zapi.template.get(filter={"name":template_name})[0]["templateid"])
    print "template id: ",template_id

    template_items = []
    ##NOTE value types: 0=flaot,3=decimal
    #miner
    template_items.append({"name":"AC2DC Current","key_":"miner.ac2dc.current", "delay":10, "value_type":0, "description":"AC2DC Current"})
    template_items.append({"name":"AC2DC Temperature","key_":"miner.ac2dc.temp", "delay":10, "value_type":0, "description":"AC2DC Temperature"})
    template_items.append({"name":"Miner - Total Cycles","key_":"miner.cycles[total]", "delay":10, "value_type":3, "description":"Total Cycles [Freq*Engines]"})

    # loops
    for l in range(24):
        #loop-level items
        template_items.append({"name":"Loop Enabled", "key_":"loop%d.enabled" % (l), "delay": 10, "value_type": 3, "description":"Loop Enabled"})
        template_items.append({"name":"Loop Current","key_":"loop%d.current" % (l), "delay":10, "value_type":0, "description":"Loop Current"})
        template_items.append({"name":"Voltage","key_":"loop%d.voltage" % (l), "delay":10, "value_type":0, "description":"Voltage"})
        template_items.append({"name":"Temperature","key_":"loop%d.temp" % (l), "delay":10, "value_type":0, "description":"Temperature"})
        template_items.append({"name":"Loop %d Average Temperature" % (l),"key_":"loop%d.temp[avg]" % (l), "delay":10, "value_type":0, "description":"Average Temperature"})
        template_items.append({"name":"Loop %d Average Frequency" % (l),"key_":"loop%d.freq[avg]" % (l), "delay":10, "value_type":0, "description":"Average Frequency"})
        template_items.append({"name":"Loop %d Total Cycles" % (l),"key_":"loop%d.cycles[total]" % (l), "delay":10, "value_type":0, "description":"Total Cycles"})
        template_items.append({"name":"Loop %d Total Failed BISTs" % (l),"key_":"loop%d.failed_bists[total]" % (l), "delay":10, "value_type":0, "description":"Total Failed BISTs"})

        for a in range(8):
            #asic-level items
            #template_items.append({"name":"Corner", "key_":"loop%d.asic%d.corner" % (l,a), "delay":10, "value_type":3, "description":"Corner"})
            template_items.append({"name":"Frequency", "key_":"loop%d.asic%d.freq" % (l,a), "delay":10, "value_type":0, "description":"Frequency"})
            template_items.append({"name":"Failed BISTs", "key_":"loop%d.asic%d.failed_bists" % (l,a), "delay":10, "value_type":0, "description":"Failed BISTs"})
            template_items.append({"name":"Frequency Time", "key_":"loop%d.asic%d.freq_time" % (l,a), "delay":10, "value_type":0, "description":"Frequency Time"})
            template_items.append({"name":"Working Engines", "key_":"loop%d.asic%d.working_engines" % (l,a), "delay":10, "value_type":3, "description":"Working Engines"})
            template_items.append({"name":"Temperature", "key_":"loop%d.asic%d.temp" % (l,a), "delay":10, "value_type":0, "description":"Temperature"})
            #template_items.append({"name":"Wins", "key_":"loop%d.asic%d.wins" % (l,a), "delay":10, "value_type":3, "description":"Wins"})
            template_items.append({"name":"Cycles", "key_":"loop%d.asic%d.cycles" % (l,a), "delay":10, "value_type":3, "description":"Cycles"})

    for t in template_items:
        t.update({"hostid":template_id, "type":2})  # 2 = zabbix_trapper (sender)

    try:
        zapi.item.create(*template_items)
        print "created %d items"%len(template_items)
    except Exception:
        print Exception
        print "creating items faild -- maybe they exist already?"

    # graphs
    # items can be a list, which will allow multiple different items to be graphed together?
    # if items is a list, settings should be a list of the same length


    #graphs
    # 1. set of graphs, showing cycles of all miners, grouped into sets of 20? ("racks")
    # 1a. as above, showing AC2DC current
    # 2. graph showing miners uptime, split as before
    # 3. miner, show ASICS temp/frequency, and miner voltage
    # 3a. miner, temperature per loop, cycles
    # 4. miner, a set of graphs (2x3), each graph showing 4 loops (=24), for each loop show cycles and temperature (2 graphs?)
    # 5. miner, AC2DC volt and current, and per-loop DC2DC current and temperature
    # 6. asic, temperature and cycles, and loop DC2DC 

    # farm-wide graphs need to be set p as hosts are added, 
    # create six graphs, each for a rande of four loops
    gids = []
    for g in range(6): # 6 grpahs, 4loops in each one, 
        items = [get_items(template_id, "temp", loop=range(g*4, g*4+4))]
        items.append(get_items(template_id, "cycles[total]", loop=range(g*4, g*4+4)))
        gids.append(create_graph("Loops %d - %d, Cycles and Temperatures" % (g*4, g*4+3), items, [{}, {"drawtype":4, "yaxisside":1}], {}))
    print "created graphs, ",gids
    # add all six graphs to a screen
    # options for items to add to screens:
		# 0 =  Graph
		# 1 =  Simple graph
		# 2 =  Map
		# 3 =  Plain text
		# 4 =  Hosts info
		# 5 =  Triggers info
		# 6 =  Server info
		# 7 =  Clock
		# 8 =  Screen
		# 9 =  Triggers overview
		# 10 =  Data overview
		# 11 =  Url
		# 12 =  History of actions
		# 13 =  History of events
		# 14 =  Host group issues
		# 15 =  System status
		# 16 =  Host issues
    s = zapi.templatescreen.create({
            "templateid": template_id,
            "name": "Cycles And Temperatures",
            "hsize": 2,
            "vsize": 3,
            "screenitems": [{"resourcetype":0, "rowspan":0,"colspan":0, "resourceid":g, "x":int(i%2 != 0), "y":i/2} for i,g in zip(range(6), gids)]
        })
    print "created screen Cycles And Temperatures, id:",int(s["screenids"][0])



    #system-wide screens and graphs
    # for now, appending group name, to allow multiple creations
    # graphs will be added dynamically when miners are added, so for now we just create an empty screen
    screens = ["%s Uptime", "%s Hertz", "%s AC2DC"]
    screens = [int(zapi.screen.create({
            "name": s % (group_name),
            "hsize": 2,
            "vsize": 3,
            })["screenids"][0]) for s in screens]
    print "created screens ", screens

main()
