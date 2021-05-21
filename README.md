# NetWatch

This is a tool to help you figure out what's using up all your data.  It's
still in extremely early stages of development.  There are three main parts:


## Data Capture

### The Watcher

This part does most of the interesting work.  It taps into your local network
connections using `libpcap` and observes the source, destination, and size of
all packets.  If you want to monitor all the traffic on your network, you'll
need to either run NetWatch on the gateway node or configure your gateway to
send copies of all packets to the machine running NetWatch.


### The DNS Proxy

Ideally, you don't just want the external IPs that are using up all your data;
you want their hostnames and domain names.  Unfortunately, reverse DNS isn't
very reliable.  So NetWatch also acts as a DNS proxy, and builds a reverse
index of all the forward DNS lookups that pass through it.  Use DHCP to set
the NetWatch machine as your network's nameserver to capture all lookups.


### The Metric Server

Now that you have all your data, how do you see it?  The final component acts
as an extremely simple HTTP server that renders the collected data in the
Prometheus metrics format in response to any request.  Once Prometheus has
your data, you can use pre-existing visualization tools to check it out.


## Visualization

### Prometheus

Prometheus is a time series database.  It's used for monitoring... things,
mainly things that have values that change over time, like CPU usage or numbers
of HTTP requests (as opposed to events, like log messages).  NetWatch uses it
to store the (cumulative) amount of data sent between (src, dst) endpoints.


### Grafana

Grafana is a visualization tool.  You use it to make dashboards with graphs on
them.  It can pull data from a variety of backends - in this case, Prometheus.
