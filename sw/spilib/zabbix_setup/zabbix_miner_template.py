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

#miner
template_items.append({"name":"AC2DC Current","key_":"ac2dc.current", "delay":10, "value_type":0, "description":"AC2DC Current"})
template_items.append({"name":"AC2DC Temperature","key_":"ac2dc.temp", "delay":10, "value_type":0, "description":"AC2DC Temperature"})

# loops
for l in range(24):
    #loop-level items
    template_items.append({"name":"Loop Enabled", "key_":"loop%d.enabled" % (l), "delay": 10, "value_type": 3, "description":"Loop Enabled"})
    #template_items.append({"name":"Loop Broken","key_":"loop%d.broken" % (l), "delay":10, "value_type":3, "description":"Loop Broken"})
    template_items.append({"name":"Loop Current","key_":"loop%d.current" % (l), "delay":10, "value_type":0, "description":"Loop Current"})
    template_items.append({"name":"Voltage","key_":"loop%d.voltage" % (l), "delay":10, "value_type":0, "description":"Voltage"})
    template_items.append({"name":"Temperature","key_":"loop%d.temp" % (l), "delay":10, "value_type":0, "description":"Temperature"})

    for a in range(8):
        #asic-level items
        #template_items.append({"name":"ASIC Enabled", "key_":"loop%d.asic%d.enabled" % (l,a), "delay":10, "value_type":3, "description":"ASIC Enabled"})
        #template_items.append({"name":"Corner", "key_":"loop%d.asic%d.corner" % (l,a), "delay":10, "value_type":3, "description":"Corner"})
        template_items.append({"name":"Frequency", "key_":"loop%d.asic%d.freq" % (l,a), "delay":10, "value_type":0, "description":"Frequency"})
        template_items.append({"name":"Failed BISTs", "key_":"loop%d.asic%d.failed_bists" % (l,a), "delay":10, "value_type":0, "description":"Failed BISTs"})
        template_items.append({"name":"Frequency Time", "key_":"loop%d.asic%d.freq_time" % (l,a), "delay":10, "value_type":0, "description":"Frequency Time"})
        template_items.append({"name":"Working Engines", "key_":"loop%d.asic%d.working_engines" % (l,a), "delay":10, "value_type":3, "description":"Working Engines"})
        #template_items.append({"name":"Engines On", "key_":"loop%d.asic%d.engines_on" % (l,a), "delay":10, "value_type":3, "description":"Engines On"})
        template_items.append({"name":"Temperature", "key_":"loop%d.asic%d.temp" % (l,a), "delay":10, "value_type":0, "description":"Temperature"})
        template_items.append({"name":"Wins", "key_":"loop%d.asic%d.wins" % (l,a), "delay":10, "value_type":3, "description":"Wins"})

for t in template_items:
    t.update({"hostid":template_id, "type":2})  # 2 = zabbix_trapper (sender)

zapi.item.create(*template_items)
print "created %d items"%len(template_items)

#calculated items
loop_items = [["loop%d.asic%d.%%s"%(l,a) for a in range(8)] for l in range(24)]
miner_items = [i for l in loop_items for i in l ]

# NOTE that the function uses externally defined variables!
# calculation is an object of form {calc:[{def:string, join:oper}], join:oper, post:string}
#   outer join operator is when calc has more than one calculation, otherwise it is ignored, but must be present
#   post is an operation to perform after composing the main function. by defailt it does nothing. it will be passed the main computation as a string, and the total number of elements as a decimal. 
#   the "post" feature is surprisingly but not coincidentally very fitting for calculating the average
def calculated_asic_graph(title, key, type, calculation):
    if not calculation.has_key("post"): calculation["post"] = "%s+%d*0"
    loop_item_ids = zapi.item.create(*[{
            "name": "Loop %d - %s" % (i, title),
            "hostid":template_id,
            "type":15,  # calculated
            "key_":"loop%d.%s" % (i,key),
            "delay": 30,
            "value_type":type,
            "params": calculation["post"] % (calculation["oper"].join(["("+c["oper"].join([c["def"]%(a%"") for a in l])+")" for c in calculation["calc"]]), len(l))
        }for i,l in zip(range(len(loop_items)), loop_items) ])
    total_item_id = zapi.item.create({
            "name": "Miner Totals - %s" % (title),
            "hostid":template_id,
            "type":15,  # calculated
            "key_":key,
            "delay":30,
            "value_type":type,
            "params":calculation["post"] % (calculation["oper"].join([c["oper"].join([c["def"]%(a%"")  for a in miner_items])for c in calculation["calc"]]), len(miner_items))
            }) 
    # graph!
    graphlines = [{
        "itemid":int(iid),
        "drawtype":"0",  # 0 = line, 1=filled, 2=bold, 3=dot, 4=dashed, 5=gradient
        "sortorder":1+c,
        "color":"".join(["%s"%"3456789abc"[((7+c)+11*(i+c))%10] for i in range(6)]),
        "yaxisside":"0", # 0=left,1=right
        "calc_fnc":"2", # 1 = min, 2 = avg, 4 = max, 7 = all
        "type":"0",  # ??
        "periods_cnt":"5"
        } for c, iid in zip(range(len(loop_item_ids["itemids"])),loop_item_ids["itemids"])]
    graphlines.append({
        "itemid":int(total_item_id["itemids"][0]),
        "drawtype":"1",  # 0 = line, 1=filled, 2=bold, 3=dot, 4=dashed, 5=gradient
        "sortorder":"0",
        "color":"ff2244",
        "yaxisside":"0", # 0=left,1=right
        "calc_fnc":"2", # 1 = min, 2 = avg, 4 = max, 7 = all
        "type":"0",  # ??
        "periods_cnt":"5"
        })
    zapi.graph.create({
        "gitems":graphlines,
        "name":"Miner Totals - %s" % (title),
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
        })
    print "created graph ",title


#wins
calculated_asic_graph("Total Wins", "wins[total]", 3, {"calc":[{ "def":"sum(%swins,30)", "oper":"+"}], "oper":""})
#avg temperature
calculated_asic_graph("Temperature Averages", "temperature[avg]", 0, {"calc":[{ "def":"avg(%stemp,30)", "oper":"+"}], "oper":"", "post":"(%s)/%d"})
#avg frequency
calculated_asic_graph("Frequency Averages", "frequency[avg]", 0, {"calc":[{ "def":"avg(%sfreq,30)", "oper":"+"}], "oper":"", "post":"(%s)/%d"})
# wins / voltage
#calculated_asic_graph("Wins/Voltage", "wins.voltage[avg]", 0, {"calc":[{ "def":"sum(%swins,30)", "oper":"+"},{"def":"sum(%svoltage,30)", "oper":"+"}], "oper":"/", "post":"(%s)/%d"})
#wins/frequency
calculated_asic_graph("Wins/Frequency", "wins.freq[avg]", 0, {"calc":[{ "def":"sum(%swins,30)", "oper":"+"},{"def":"sum(%sfreq,30)", "oper":"+"}], "oper":"/", "post":"(%s)/%d"})
# voltage/frequency
#calculated_asic_graph("Voltage/Frequency", "voltage.freq[avg]", 0, {"calc":[{ "def":"sum(%svoltage,30)", "oper":"+"},{"def":"sum(%sfreq,30)", "oper":"+"}], "oper":"/", "post":"(%s)/%d"})

