home; clear
format compact
commandwindow

expData = getExperimentData('test');
noVesselData = getExperimentData('ControlSkinvessel'); %second plot

% expData.name = '<REPLACE_ME_FOR_GRAPHS>'; %Name displayed on graph
% noVesselData.name = '<REPLACE_ME_FOR_GRAPHS>'; %Name displayed on graph



PLOT = 4; % Plot = 1 Plot first experiment
          % Plot = 2 Plot first and second experiment
          % Plot = 3 Plot First experiment subtracted by second experiment
          % Plot = 4 CMOS display fluence read by CMOS sensor
          % Plot = 5 Spectral analyzer
          
TITLE_FONT_SIZE = 12;
AXES_FONT_SIZE = 6;

SAVEPICSON = 0; %1 or 0 

if SAVEPICSON
    sz = 10; fz = 7; fz2 = 5; % to use savepic.m
else
    sz = 12; fz = 9; fz2 = 7; % for screen display
end

BV_2D = squeeze(expData.FluenceArray(:,:,expData.Nz));
if PLOT <=2
    figure;clf
    if PLOT==1
        subplot(1,2,1)
    else
        subplot(2,2,1)
    end
    BV_2D_Percent = BV_2D/(sum(sum(expData.FluenceArray(:,:,1))))*100;
    imagesc(expData.x,expData.y,BV_2D_Percent)%F(x,y,z) 
    hold on
    colorbar
    xlabel('x [cm]')
    ylabel('y [cm]')
    title(strcat(expData.name,' % Fluence At Bottom of Tissue, t= ',string(expData.time_min),'min'),'FontSize',TITLE_FONT_SIZE)
    set(gca,'FontSize',AXES_FONT_SIZE)
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
    set(gca,'FontSize',AXES_FONT_SIZE)
end
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
    set(gca,'FontSize',AXES_FONT_SIZE)
    colormap(makec2f)
    axis equal image

    subplot(2,2,4)
    Temp2 = sum(NBV_2D,1)/(sum(sum(expData.FluenceArray(:,:,1))))*100;
    plot(noVesselData.x,Temp2)
    xlabel('x [cm]')
    ylabel('Sum of % Fluence along y')
    title(strcat(noVesselData.name,' % Fluence, t= ',string(noVesselData.time_min),'min'), 'FontSize',TITLE_FONT_SIZE)
    set(gca,'FontSize',AXES_FONT_SIZE)
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

if PLOT==4 %CMOS
    figure;
    CMOS_Array = BV_2D(99:100, 128:1151);
    subplot(1,2,1)
    imagesc(CMOS_Array)
    xlabel('x [cm]')
    ylabel('y [cm]')
    title(strcat(expData.name,' % Fluence Seen by CMOS, t= ',string(expData.time_min),'min'),'FontSize',TITLE_FONT_SIZE)
    %Find area of sensor
    %dx,y,z = dize of pixel
    %G9494-256D CMOS --> 50x50µm pixels 256 pixels 1280µm 1.28 cm total
    %2 (in y) by 512 (x)
    %F(y,x,z) y -->200, x-->1280
    subplot(1,2,2)
    CMOS_Linear = sum(CMOS_Array,1);
    plot(expData.x(128:1151),CMOS_Linear)
    title(strcat(expData.name,' % Fluence Seen By CMOS, t= ',string(expData.time_min),'min'), 'FontSize',TITLE_FONT_SIZE)
    xlabel('x [cm]')
    ylabel('% Fluence')
    
end

if SAVEPICSON
    name = sprintf('%s_2Fluence.jpg',expData.name);
    savepic(1,[10 5],name)
end