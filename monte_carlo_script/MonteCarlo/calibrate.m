% lookmcxyz.m
%   Looks at myname_F.bin, created by mcxyz.c 
%   where myname is the name of the run: myname_T.bin, myname_H.mci
%   Makes figures:
%       myname_tissue.jpg   = tissue structure (shows tissue types)
%       myname_Fzx.jpg      = fluence rate vs z,x
%       myname_Fzy.jpg      = fluence rate vs z,y
%   Uses:
%       myname_H.mci    = input file from maketissue.m
%       myname_T.bin    = tissue input file from maketissue.m
%       myname_F.bin    = fluence rate output from Monte Carlo
%       reportH_mci.m   = lists input parameters in myname_H.mci
%       makecmap.m      = makes colormap for tissue types
%       makec2f.m       = makes colormap for fluence rate
%
%   This example sets myname = 'skinvessel'.
%
% 7/feb/2017, add boundaryflag (see A(10)).
% 1/june/2017 , no major changes, just clean up display outputs.
% Steven L Jacques
home; clear
format compact
commandwindow

SAVEPICSON = 0;
if SAVEPICSON
    sz = 10; fz = 7; fz2 = 5; % to use savepic.m
else
    sz = 12; fz = 9; fz2 = 7; % for screen display
end

%%%% USER CHOICES <---------- you must specify -----
myname = 'lowSkinvessel'; nm = 532;
controlname = 'ControlSkinvessel'; nm = 532;
%%%%


disp(sprintf('------ mcxyz %s -------',myname))

% Load header file of experiment
filename = sprintf('%s_H.mci',myname);
disp(['loading ' filename])
fid = fopen(filename, 'r');
A = fscanf(fid,'%f',[1 Inf])';
fclose(fid);

% Load header file of control
filenameC = sprintf('%s_H.mci',controlname);
disp(['loading ' filenameC])
fid = fopen(filenameC, 'r');
B = fscanf(fid,'%f',[1 Inf])';
fclose(fid);

%% parameters of experiment 
time_min = A(1);
Nx = A(2);
Ny = A(3);
Nz = A(4);
dx = A(5);
dy = A(6);
dz = A(7);
mcflag = A(8);
launchflag = A(9);
boundaryflag = A(10);
xs = A(11);
ys = A(12);
zs = A(13);
xfocus = A(14);
yfocus = A(15);
zfocus = A(16);
ux0 = A(17);
uy0 = A(18);
uz0 = A(19);
radius = A(20);
waist = A(21);
Nt = A(22);
j = 22;
for i=1:Nt %for every tissue type add parameters (absorption, scattering, anisotropy of scattering) to A list 
    j=j+1;
    muav(i,1) = A(j);
    j=j+1;
    musv(i,1) = A(j);
    j=j+1;
    gv(i,1) = A(j);
end

reportHmci(myname)

%% parameters of control 
time_min = B(1);
NxC = B(2);
NyC = B(3);
NzC = B(4);
dxC = B(5);
dyC = B(6);
dzC = B(7);
mcflagC = B(8);
launchflagC = B(9);
boundaryflagC = B(10);
xsC = B(11);
ysC = B(12);
zsC = B(13);
xfocusC = B(14);
yfocusC = B(15);
zfocusC = B(16);
ux0C = B(17);
uy0C = B(18);
uz0C = B(19);
radiusC = B(20);
waistC = B(21);
NtC = B(22);
j = 22;
for i=1:NtC %for every tissue type add parameters (absorption, scattering, anisotropy of scattering) to A list 
    j=j+1;
    muav(i,1) = B(j);
    j=j+1;
    musv(i,1) = B(j);
    j=j+1;
    gv(i,1) = B(j);
end

reportHmci(controlname)

%% Load Fluence rate F(y,x,z) of experiment
filename = sprintf('%s_F.bin',myname);
disp(['loading ' filename])
tic
    fid = fopen(filename, 'rb');
    [Data count] = fread(fid, Ny*Nx*Nz, 'float');
    fclose(fid);
toc
F = reshape(Data,Ny,Nx,Nz); % F(y,x,z)

%% Load Fluence rate F(y,x,z) of control
filenameC = sprintf('%s_F.bin',controlname);
disp(['loading ' filename])
tic
    fidC = fopen(filenameC, 'rb');
    [DataC countC] = fread(fidC, NyC*NxC*NzC, 'float');
    fclose(fidC);
toc
FC = reshape(DataC,NyC,NxC,NzC); % F(y,x,z)

%%
% Load tissue structure in voxels, T(y,x,z)for experiment
filename = sprintf('%s_T.bin',myname);
disp(['loading ' filename])
tic
    fid = fopen(filename, 'rb');
    [Data count] = fread(fid, Ny*Nx*Nz, 'uint8');
    fclose(fid);
toc
T = reshape(Data,Ny,Nx,Nz); % T(y,x,z)

clear Data

%%
% Load tissue structure in voxels, T(y,x,z)for control
filenameC = sprintf('%s_T.bin',controlname);
disp(['loading ' filenameC])
tic
    fidC = fopen(filenameC, 'rb');
    [DataC countC] = fread(fidC, NyC*NxC*NzC, 'uint8');
    fclose(fidC);
toc
TC = reshape(DataC,NyC,NxC,NzC); % T(y,x,z)

clear Data

%%
x = ([1:Nx]-Nx/2-1/2)*dx;
y = ([1:Ny]-Ny/2-1/2)*dx;
z = ([1:Nz]-1/2)*dz;
ux = [2:Nx-1]; %photon trajectory as cosines
uy = [2:Ny-1];
uz = [2:Nz-1];
zmin = min(z);
zmax = max(z);
zdiff = zmax-zmin;
xmin = min(x);
xmax = max(x);
xdiff = xmax-xmin;


%% look Fxy (Bottom of tissue)
%F(y,x,z)
%Fxy = reshape(F(:,:,Nz),Nx,Ny)';
%FxyC = reshape(FC(:,:,NzC),NyC,NzC)';
Fxy = F(:,:,Nz);
FxyC = FC(:,:,NzC);
iy = round((dy*Ny/2 + 0.15)/dy);
iz = round(zs/dz);
zzs  = zs;
%Fdet = mean(reshape(Fzy(iz+[-1:1],iy+[0 1]),6,1));

figure;clf
subplot(1,2,1)
imagesc(x,y,log10(F(:,:,Nz)),[.5 2.8])
hold on
text(max(x)*1.2,min(z)-0.04*max(z),'log_{10}( \phi )','fontsize',fz)
colorbar
set(gca,'fontsize',sz)
xlabel('x [cm]')
ylabel('y [cm]')
title('Fluence \phi [W/cm^2/W.delivered] Low Vessel','fontweight','normal','fontsize',fz)
colormap(makec2f)
axis equal image
text(min(x)-0.2*max(x),min(z)-0.08*max(z),sprintf('runtime = %0.1f min',time_min),...
    'fontsize',fz2)

if SAVEPICSON
    name = sprintf('%s_Fxy.jpg',myname);
    savepic(4,[4 3],name)
end
subplot(1,2,2)
hold on
imagesc(x,y,log10(FC(:,:,NzC)),[.5 2.8])
text(max(x)*1.2,min(z)-0.04*max(z),'log_{10}( \phi )','fontsize',fz)
colorbar
set(gca,'fontsize',sz)
xlabel('x [cm]')
ylabel('y [cm]')
title('Fluence \phi [W/cm^2/W.delivered] No Vessel','fontweight','normal','fontsize',fz)
colormap(makec2f)
axis equal image
text(min(x)-0.2*max(x),min(z)-0.08*max(z),sprintf('runtime = %0.1f min',time_min),...
    'fontsize',fz2)


figure;clf
subplot(1,2,1)
imagesc(x,y,(F(:,:,Nz)-FC(:,:,NzC)))
title('Vessel Fluence - Control Fluence (No Vessel)')
subplot(1,2,2)
%Temp = log10(Fxy);
%F(y,x,z)
Temp =(Fxy-FxyC);
Lin = sum(Temp,1);
norm_Lin = (Lin - min(Lin)) / ( max(Lin) - min(Lin) )
hold on
title('Normalized Flunece')
plot(x,norm_Lin)
f = polyfit(x,norm_Lin,2)
plot(x,polyval(f,x))
% 
% figure(6)
% plot(x,Lin)

figure;clf
subplot(1,2,1)
imagesc(x,y,(Fxy))
title('Experimnet Fluence')
subplot(1,2,2)
%Temp = log10(Fxy);
%F(y,x,z)
Temp1 =(Fxy);
Lin1 = sum(Temp1,1);
norm_Lin1 = (Lin1 - min(Lin1)) / ( max(Lin1) - min(Lin1) )
hold on
title('Experimnet Fluence Normalized')
plot(x,norm_Lin1)
f1 = polyfit(x,norm_Lin1,4);
plot(x,polyval(f1,x))

drawnow

disp('done')


