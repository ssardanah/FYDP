% Load Fluence data with important parameters into matlab

function s = getExperimentData(myname)

s = struct;

filename = sprintf('%s_H.mci',myname);
disp(['loading ' filename])
fid = fopen(filename, 'r');
A = fscanf(fid,'%f',[1 Inf]);
fclose(fid);

%Get parameters
s.time_min = A(1);
s.Nx = A(2);
s.Ny = A(3);
s.Nz = A(4);
s.dx = A(5);
s.dy = A(6);
s.dz = A(7);
s.mcflag = A(8);
s.launchflag = A(9);
s.boundaryflag = A(10);
s.xs = A(11);
s.ys = A(12);
s.zs = A(13);
s.xfocus = A(14);
s.yfocus = A(15);
s.zfocus = A(16);
s.ux0 = A(17);
s.uy0 = A(18);
s.uz0 = A(19);
s.radius = A(20);
s.waist = A(21);
s.Nt = A(22);
j = 22;

% for i=1:Nt %for every tissue type add parameters (absorption, scattering, anisotropy of scattering) to A list 
%     j=j+1;
%     muav(i,1) = A(j);
%     j=j+1;
%     musv(i,1) = A(j);
%     j=j+1;
%     gv(i,1) = A(j);
% end
%% Load Fluence rate F(y,x,z) of experiment
filename = sprintf('%s_F.bin',myname);
disp(['loading ' filename])
tic
    fid = fopen(filename, 'rb');
    [Data count] = fread(fid, s.Ny*s.Nx*s.Nz, 'float');
    fclose(fid);
toc
s.FluenceArray = reshape(Data,s.Ny,s.Nx,s.Nz); % F(y,x,z)

s.x = ([1:s.Nx]-s.Nx/2-1/2)*s.dx;
s.y = ([1:s.Ny]-s.Ny/2-1/2)*s.dx;
s.z = ([1:s.Nz]-1/2)*s.dz;

