% 

home; clear
format compact
commandwindow

expData = getExperimentData('skinvesselSteph3'); % Shifted to be F(x,y,z)
noVesselData = getExperimentData('ControlSkinvessel');
%skinvesselSteph


PLOT = 1; % Plot = 1 Plot first experiment
         % Plot = 2 Plot first and second experiment
         % Plot = 3 Plot First experiment subtracted by second experiment

SAVEPICSON = 0;
if SAVEPICSON
    sz = 10; fz = 7; fz2 = 5; % to use savepic.m
else
    sz = 12; fz = 9; fz2 = 7; % for screen display
end

figure;clf
if PLOT==1
    subplot(1,2,1)
else
    subplot(2,2,1)
end

BV_2D = squeeze(expData.FluenceArray(:,:,expData.Nz/2));
imagesc(expData.x,expData.y,((BV_2D)))%F(x,y,z) 
hold on
colorbar
xlabel('x [cm]')
ylabel('y [cm]')
title('BV Fluence At Top, t=10min')
colormap(makec2f)
%axis equal image

if PLOT==1
    subplot(1,2,2)
else
    subplot(2,2,2)
end
Temp = sum(BV_2D,1);
plot(expData.x,Temp)
Temp1 = expData.FluenceArray(:,:,1);
xlabel('x [cm]')
ylabel('Sum of Fluence in y-dir')
title('BV Fluence, t=10min') 


%Plots to compare t=10 min
if PLOT==2
    subplot(2,2,3)
    NBV_2D = noVesselData.FluenceArray(:,:,noVesselData.Nz/1.6);
    imagesc(noVesselData.y,noVesselData.x,log10(NBV_2D),[.5 2.8])
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title('Adipose Fluence, t=10min')
    colormap(makec2f)
    axis equal image

    subplot(2,2,4)
    Temp2 = sum(NBV_2D,1);
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of Fluence in y-dir')
    title('Adipose Fluence, t=10min') 
end

if PLOT==2
    subplot(2,2,3)
    NBV_2D = noVesselData.FluenceArray(:,:,noVesselData.Nz);
    imagesc(noVesselData.x,noVesselData.y,log10(NBV_2D),[.5 2.8])
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title('Adipose Fluence, t=10min')
    colormap(makec2f)
    axis equal image

    subplot(2,2,4)
    Temp2 = sum(NBV_2D,1);
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of Fluence in y-dir')
    title('Adipose Fluence, t=10min')
end
if PLOT==3
    figure;clf
    subplot(1,2,1)
    BV_2D = squeeze(expData.FluenceArray(:,:,expData.Nz));
    NBV_2D = noVesselData.FluenceArray(:,:,noVesselData.Nz);
    imagesc(noVesselData.x,noVesselData.y,(BV_2D-NBV_2D),[.5 2.8])
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title('XXXXX, t=10min')
    colormap(makec2f)
    axis equal image

    subplot(1,2,2)
    Temp1 = BV_2D-NBV_2D;
    Temp2 = sum(Temp1,1);
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of Fluence in y-dir')
    title('XXXXX, t=10min') 
end
