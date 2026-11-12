function T=fmm(F,src,t0,display)

if ~exist('display') 
    display=0;
end

tic;
T=fmmMex(F,size(F,1),size(F,2),src(2),src(1),t0,display);
t=toc;

m=exp(20);
T(T>m)=nan;

fprintf('\nFMM on image size (%i,%i) took %.3f seconds\n',size(F,1),size(F,2),t);



