import pandas
import matplotlib.pyplot as plt
import seaborn

with open('data.txt') as file:
	data = file.read().split('\n')

x = [int(d[:d.find(',')]) for d in data if len(d) > 0]
y = [int(d[d.find(',')+1:]) for d in data if len(d) > 0]
data = pandas.DataFrame(data={'x': x, 'y': y})

seaborn.lmplot(data=data, x='x', y='y')
plt.show()

