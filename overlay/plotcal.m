load -ascii upper.mat
load -ascii lower.mat

x=upper(:,1);

ru=upper(:,2);
rl=lower(:,2);
gu=upper(:,3);
gl=lower(:,3);
bu=upper(:,4);
bl=lower(:,4);

figure(1)
clf
hold on
plot(x,ru,'r');
plot(x,gu,'g');
plot(x,bu,'b');
hold off

figure(2)
clf

hold on
plot(upper(:,1),gu,'+');
plot(lower(:,1),gl,'r+');

PL=polyfit(lower(:,1),gl,1);
PU=polyfit(upper(:,1),gu,1);
hold on 
x2=min(upper(:,1)):255;
plot(x2,polyval(PU,x2),'b')
plot(x2,polyval(PL,x2),'r')

sprintf('Ex: 150-200 => 0x%02x%02x\n',round(polyval(PU,147)),round(polyval(PL,147)))