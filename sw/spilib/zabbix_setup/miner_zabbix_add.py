"""
$1 = miner name
$2 = miner ip
$3 = template
groups will be same as template
1. create host
2. link host to Base Data Template
"""

from pyzabbix import ZabbixAPI
import sys

try:
    name = sys.argv[1]
    ip = sys.argv[2]
    template = sys.argv[3]
except Exception:
    print "Usage: %s host_name ip template" % sys.argv[0]
    sys.exit()


interface ={
            "type": 1,
            "main": 1,
            "useip": 1,
            "ip": ip,
            "dns": "",
            "port": "10050"
            }

server = "http://127.0.0.1/zabbix"
user = "Admin"
pw = "zabbix"
zapi = ZabbixAPI(server)

# Login to the Zabbix API
zapi.login(user, pw)


template_id = zapi.template.get(filter={"name":template}, output="hostid,groupids")[0]["templateid"]
print "template id: ",template_id
host_group_id = zapi.hostgroup.get(templateids=[template_id]})[0]["groupid"]
print "group id: ",host_group_id

# create host
if not zapi.host.exists(name=name):
    zapi.host.create(host=name,interfaces=[interface],groups=[{"groupid": host_group_id}], templates = [{"templateid":template_id}])
host = zapi.host.get(filter={"name":name})[0]
print "new host id: ", int(host["hostid"])

# update system screens
# for each screen, check if last graph has max items (or no grpah was created yet) if yes, create new graph and add it to screen.
# add miner data to graph

