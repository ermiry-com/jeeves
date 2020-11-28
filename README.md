# Ermiry's Jeeves Service

### Development
```
sudo docker run \
  -it \
  --name jeeves --rm \
  -p 5000:5000 \
  -v /home/ermiry/Documents/ermiry/Projects/jeeves:/home/jeeves \
  -e CURR_ENV=development \
  -e PORT=5000 \
  -e PRIV_KEY=/home/jeeves/keys/key.key -e PUB_KEY=/home/jeeves/keys/key.pub \
  -e MONGO_APP_NAME=jeeves -e MONGO_DB=ermiry \
  -e MONGO_URI=mongodb://jeeves:password@192.168.100.39:27017/ermiry \
  -e CERVER_RECEIVE_BUFFER_SIZE=4096 -e CERVER_TH_THREADS=4 \
  -e CERVER_CONNECTION_QUEUE=4 \
  -e ENABLE_USERS_ROUTES=TRUE \
  ermiry/jeeves:development /bin/bash
```

## Routes

#### GET /api/jeeves
**Access:** Public \
**Description:** Jeeves top level route \
**Returns:**
  - 200 on success

#### GET api/jeeves/version
**Access:** Public \
**Description:** Returns jeeves service current version \
**Returns:**
  - 200 and version's json on success

#### GET api/jeeves/auth
**Access:** Private \
**Description:** Used to test if jwt keys work correctly \
**Returns:**
  - 200 on success
  - 401 on failed auth