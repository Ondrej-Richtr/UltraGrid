#!/bin/sh

if expr $GITHUB_REF : 'refs/heads/release/'; then
  VERSION=${GITHUB_REF#refs/heads/release/}
  TAG=v$VERSION
elif [ $GITHUB_REF = 'refs/heads/ndi-build' ]; then
  VERSION=ndi
  TAG=$VERSION
else
  VERSION=continuous
  TAG=$VERSION
fi

export VERSION TAG

echo "::set-env name=VERSION::$VERSION"
echo "::set-env name=TAG::$TAG"
