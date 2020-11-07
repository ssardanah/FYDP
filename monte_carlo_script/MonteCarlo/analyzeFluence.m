home; clear
format compact
commandwindow

expData = getExperimentData('lowSkinvessel');
noVesselData = getExperimentData('ControlSkinvessel'); 

% expData.name = '<REPLACE_ME_FOR_GRAPHS>';
% noVesselData.name = '<REPLACE_ME_FOR_GRAPHS>';


PLOT = 1; % Plot = 1 Plot first experiment
          % Plot = 2 Plot first and second experiment
          % Plot = 3 Plot First experiment subtracted by second experiment
          % Plot = CMOS display fluence read by CMOS sensor
TITLE_FONT_SIZE = 12;
AZES_FONT_SIZE = 6;
SAVEPICSON = 1;
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

BV_2D = squeeze(expData.FluenceArray(:,:,expData.Nz));
BV_2D_Percent = BV_2D/(sum(sum(expData.FluenceArray(:,:,1))))*100;
imagesc(expData.x,expData.y,BV_2D_Percent)%F(x,y,z) 
hold on
colorbar
xlabel('x [cm]')
ylabel('y [cm]')
title(strcat(expData.name,' % Fluence At Bottom of Tissue, t= ',string(expData.time_min),'min'),'FontSize',TITLE_FONT_SIZE)
set(gca,'FontSize',AZES_FONT_SIZE)
colormap(makec2f)
%axis equal image

if PLOT==1
    subplot(1,2,2)
else
    subplot(2,2,2)
end
Temp = sum(BV_2D,1)/(sum(sum(expData.FluenceArray(:,:,1))))*100;
plot(expData.x,Temp)
xlabel('x [cm]')
ylabel('Sum of % Fluence along y')
title(strcat(expData.name,' % Fluence, t= ',string(expData.time_min),'min'),'FontSize',TITLE_FONT_SIZE)
set(gca,'FontSize',AZES_FONT_SIZE)

%Plots to compare t=10 min
if PLOT==2
    subplot(2,2,3)
    NBV_2D = noVesselData.FluenceArray(:,:,noVesselData.Nz);
    NBV_2D_Percent = noVesselData.FluenceArray(:,:,noVesselData.Nz)/(sum(sum(expData.FluenceArray(:,:,1))))*100;
    imagesc(noVesselData.y,noVesselData.x,NBV_2D_Percent)
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title(strcat(noVesselData.name,' % Fluence At Bottom of Tissue, t= ',string(noVesselData.time_min),'min'), 'FontSize',TITLE_FONT_SIZE)
    set(gca,'FontSize',AZES_FONT_SIZE)
    colormap(makec2f)
    axis equal image

    subplot(2,2,4)
    Temp2 = sum(NBV_2D,1)/(sum(sum(expData.FluenceArray(:,:,1))))*100;
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of % Fluence along y')
    title(strcat(noVesselData.name,' % Fluence, t= ',string(noVesselData.time_min),'min'), 'FontSize',TITLE_FONT_SIZE)
    set(gca,'FontSize',AZES_FONT_SIZE)
end

if PLOT==3
    %Incomplete to develop if needed 
    figure;clf
    subplot(1,2,1)
    BV_2D = squeeze(expData.FluenceArray(:,:,expData.Nz));
    NBV_2D = noVesselData.FluenceArray(:,:,noVesselData.Nz);
    imagesc(noVesselData.x,noVesselData.y,(BV_2D-NBV_2D),[.5 2.8])
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title('INCOMPLETE CODE')
    colormap(makec2f)
    axis equal image

    subplot(1,2,2)
    Temp1 = BV_2D-NBV_2D;
    Temp2 = sum(Temp1,1);
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of Fluence in y-dir')
    title('INCOMPLETE CODE') 
end

if SAVEPICSON
    name = sprintf('%s_2Fluence.jpg',expData.name);
    savepic(1,[10 5],name)
end