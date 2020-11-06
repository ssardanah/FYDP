% 

home; clear
format compact
commandwindow

t100SV = getExperimentData('skinvessel');
t10SV = getExperimentData('lowSkinvessel');

%skinvesselSteph
X = t100SV.FluenceArray(:,1,200);

SAVEPICSON = 0;
if SAVEPICSON
    sz = 10; fz = 7; fz2 = 5; % to use savepic.m
else
    sz = 12; fz = 9; fz2 = 7; % for screen display
end

figure;clf
subplot(2,2,1)
Temp = squeeze(t100SV.FluenceArray(:,:,t100SV.Nz));
imagesc(t100SV.x,t100SV.z,log10((Temp)),[.5 2.8])%F(y,x,z) 
hold on
colorbar
xlabel('y [cm]')
ylabel('x [cm]')
title('BV Fluence, t=10min')
colormap(makec2f)
axis equal image

subplot(2,2,2)
Temp2 = sum(Temp,1);
plot(t100SV.x,Temp2)
Temp1 = t100SV.FluenceArray(:,:,1);
colorbar
xlabel('x [cm]')
ylabel('Sum of Fluence in y-dir')
title('BV Fluence, t=10min') 
colormap(makec2f)


%Plots to compare t=10 min
subplot(2,2,3)
Temp = t10SV.FluenceArray(:,:,t10SV.Nz);
imagesc(t10SV.x,t10SV.y,log10(Temp),[.5 2.8])
hold on
colorbar
xlabel('x [cm]')
ylabel('y [cm]')
title('BV Fluence, t=100min')
colormap(makec2f)
axis equal image

subplot(2,2,4)
Temp2 = sum(Temp,1);
plot(t10SV.x,Temp2)
colorbar
xlabel('x [cm]')
ylabel('Sum of Fluence in y-dir')
title('BV Fluence, t=100min') 
colormap(makec2f)

