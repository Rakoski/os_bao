```c++
S= set();
QPage = Queue();
PF=0;
for k = 0 to length (N) do
	if length (S) < C then
		if P[k] not in S then
			S.add(P[k]);
			PF = PF + 1;
			QPage.put(P[k]);
		end
	else        
		if P[k] not in S then
			val = QPage.queue[0];
			QPage.get();
			S.remove(val);
			S.add(P[k]);
			QPage.put(P[k]);
			PF = PF + 1;
		end
	end
end
return PF;
```