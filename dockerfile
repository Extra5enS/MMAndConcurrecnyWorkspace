# Use the official Ubuntu base image
FROM ubuntu:latest

# Set environment variables to avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package lists and install essential packages
RUN apt-get update
RUN apt install build-essential -y
RUN apt install curl -y
RUN apt install wget -y
RUN apt install git -y
RUN apt install vim -y
RUN apt install sudo -y
RUN apt install ninja-build -y
RUN apt install gdb -y
RUN apt install clang-14 -y
RUN apt install clang-tidy-14 -y
RUN apt install cmake -y
RUN apt install clang-format-14 -y
RUN apt install clangd-14 -y

# Set the working directory
WORKDIR /app

# Copy the local files to the container
COPY . /app

# Expose a port (optional, if you are running a service)
# EXPOSE 8080

# Command to run when the container starts (optional)
# CMD ["bash"]
