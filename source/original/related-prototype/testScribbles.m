I=imread('image.jpg');
load('scrib1.mat');
imshow(I);
hold on
plot(a(:,1),a(:,2),'w.');
Ibw=logical(0*rgb2gray(I));
