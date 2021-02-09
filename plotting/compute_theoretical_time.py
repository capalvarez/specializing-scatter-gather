import csv

rtt = 170
workers = [100*i for i in range(1, 21)]
workers = [1, *workers]

payload = [1000, 16000, 64000, 256000]
mss = 1380.0
mtu = 1500.0
bandwidth = 10

ideal_jct = []

for w in workers:
    for size in payload:
        jct = rtt * 1000 + 8 * 74.0 / bandwidth + (8 * size * mtu * w / mss) / bandwidth
        ideal_jct.append({'workers': w, 'payload': size, 'jct': jct})


with open('theory_jct.csv', 'w+', newline='') as f:
    fieldnames = ['workers', 'payload', 'fct']
    writer = csv.writer(f, delimiter=',')

    writer.writerow(fieldnames)

    for jct in ideal_jct:
        writer.writerow([jct['workers'], jct['payload'], jct['jct'] / 1000000])