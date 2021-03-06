# Ermiry's Jeeves Service

### Development
```
sudo docker run \
  -it \
  --name api --rm \
  -p 5000:5000 --net jeeves \
  --mount type=tmpfs,destination=/var/uploads,tmpfs-size=256m \
  -v /home/ermiry/Documents/ermiry/Projects/jeeves-web/jeeves:/home/jeeves \
  -v /home/ermiry/Documents/ermiry/Projects/jeeves-web/uploads:/home/jeeves/uploads \
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

### Demo
```
sudo docker run \
  -it \
  --name api --rm \
  -p 5000:5000 --net jeeves \
  --mount type=tmpfs,destination=/var/uploads,tmpfs-size=256m \
  -v /home/ermiry/Documents/ermiry/Projects/jeeves-web/jeeves:/home/jeeves \
  -v /home/ermiry/Documents/ermiry/Projects/jeeves-web/uploads:/home/jeeves/uploads \
  -e CURR_ENV=demo \
  -e PORT=5000 \
  -e PRIV_KEY=/home/jeeves/keys/key.key -e PUB_KEY=/home/jeeves/keys/key.pub \
  -e MONGO_APP_NAME=jeeves -e MONGO_DB=ermiry \
  -e MONGO_URI=mongodb://jeeves:password@192.168.100.39:27017/ermiry \
  -e CERVER_RECEIVE_BUFFER_SIZE=4096 -e CERVER_TH_THREADS=4 \
  -e CERVER_CONNECTION_QUEUE=4 \
  -e ENABLE_USERS_ROUTES=TRUE \
  ermiry/jeeves:demo /bin/bash
```

## Routes

### Main

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
  - 500 on server error

### Jobs

#### GET api/jeeves/jobs
**Access:** Private \
**Description:** Returns all the user's jobs \
**Returns:**
  - 200 on success
  - 401 on failed auth
  - 500 on server error

#### POST api/jeeves/jobs
**Access:** Private \
**Description:** A user has requested to create a new job \
**Returns:**
  - 200 on success
  - 400 on bad request
  - 401 on failed auth
  - 500 on server error

#### GET api/jeeves/jobs/test
**Access:** Public \
**Description:** Jobs test route \
**Returns:**
  - 200 on success

#### GET api/jeeves/jobs/:id/info
**Access:** Private \
**Description:** A user has requested more info of a single job \
**Returns:**
  - 200 on success
  - 401 on failed auth
  - 500 on server error

#### POST api/jeeves/jobs/:id/config
**Access:** Private \
**Description:** Request to update job's configuration \
**Returns:**
  - 200 on success
  - 400 on bad request
  - 401 on failed auth
  - 500 on server error

#### POST api/jeeves/jobs/:id/upload
**Access:** Private \
**Description:** Request to add images to be processed by job \
**Returns:**
  - 200 on success
  - 400 on bad request
  - 401 on failed auth
  - 500 on server error

#### GET api/jeeves/jobs/:id/start
**Access:** Private \
**Description:** A user has requested to start a job \
**Returns:**
  - 200 on success
  - 400 on bad request
  - 401 on failed auth
  - 500 on server error

#### GET api/jeeves/jobs/:id/stop
**Access:** Private \
**Description:** A user has requested to stop a job \
**Returns:**
  - 200 on success
  - 400 on bad request
  - 401 on failed auth
  - 500 on server error

### Users

#### GET /api/users
**Access:** Public \
**Description:** Users top level route \
**Returns:**
  - 200 on success

#### POST api/users/login
**Access:** Public \
**Description:** Uses the user's supplied creedentials to perform a login and generate a jwt token \
**Returns:**
  - 200 and token on success authenticating user
  - 400 on bad request due to missing values
  - 404 on user not found
  - 500 on server error

#### POST api/users/register
**Access:** Public \
**Description:** Used by users to create a new account \
**Returns:**
  - 200 and token on success creating a new user
  - 400 on bad request due to missing values
  - 500 on server error