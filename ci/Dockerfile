# Use the official Python 3.9 image as the base
FROM python:3.9-slim

# Set the working directory
WORKDIR /workspace

RUN apt-get update && apt-get install -y git

# Install PlatformIO
RUN pip install platformio

WORKDIR /usr/src

CMD ["platformio", "run"]

